#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include "common.hh"
#include "render.hh"
#include "vec.hh"
#include "camera.hh"
#include "commonmath.hh"
#include "framemem.hh"
#include "noise.hh"
#include "tweaker.hh"
#include "menu.hh"
#include "matrix.hh"
#include "font.hh"
#include "frame.hh"
#include "debugdraw.hh"
#include "task.hh"
#include "ui.hh"
#include "gputask.hh"
#include "timer.hh"
#include "hyper.hh"
#include "htexdb.hh"

////////////////////////////////////////////////////////////////////////////////
// file scope globals

// cameras
static std::shared_ptr<Camera> g_curCamera;
static std::shared_ptr<Camera> g_mainCamera;	
static std::shared_ptr<Camera> g_debugCamera;

// main camera variables, used for saving/loading the camera
static vec3 g_focus, g_defaultFocus, g_defaultEye, g_defaultUp;

// display toggles
static int g_menuEnabled;
static int g_wireframe;
static int g_debugDisplay;

// orbit cam
static bool g_orbitCam = false;
static float g_orbitAngle;
static vec3 g_orbitFocus;
static vec3 g_orbitStart;
static float g_orbitRate;
static float g_orbitLength;

// debug cam
static vec3 g_fakeFocus = {-10.f, 0.f, 0.f};

// fps tracking
static float g_fpsDisplay;
static int g_frameCount;
static int g_frameSampleCount;
static Timer g_frameTimer;

// dt tracking
static float g_dt;
static Clock g_timer;

// lighting
static Color g_sunColor;
static vec3 g_sundir;

// control vars for debug texture render
static GLuint g_debugTexture;
static bool g_debugTextureSplit;

// screenshots & recording
static bool g_screenshotRequested = false;
static bool g_recording = false;
static int g_recordFps = 30;
static int g_recordCurFrame;
static int g_recordFrameCount = 300;
static Limits<float> g_recordTimeRange;

// demo specific stuff:

// The current hypertexture we're rendering, and a global shapes menu for adding new shapes.
static std::shared_ptr<AnimatedHypertexture> g_curHtex;
static std::shared_ptr<SubmenuMenuItem> g_shapesMenu;

// list of all hypertextures
static std::vector<std::shared_ptr<AnimatedHypertexture>> g_htexList;

// geom for the ground (just a plane)
static std::shared_ptr<Geom> g_groundGeom;

////////////////////////////////////////////////////////////////////////////////
// Shaders
static std::shared_ptr<ShaderInfo> g_groundShader;

enum GroundUniformLocType {
	GRNDBIND_ShadowMatrix,
	GRNDBIND_ShadowMap,
};

static std::vector<CustomShaderAttr> g_groundUniforms =
{
	{ GRNDBIND_ShadowMatrix, "matShadow" },
	{ GRNDBIND_ShadowMap, "shadowMap" },
};

////////////////////////////////////////////////////////////////////////////////
// forward decls
static void record_Start();

////////////////////////////////////////////////////////////////////////////////
// tweak vars - these are checked into git
extern float g_tileDrawErrorThreshold;
static std::vector<std::shared_ptr<TweakVarBase>> g_tweakVars = {
	std::make_shared<TweakVector>("cam.eye", &g_defaultEye, vec3(8.f, 0.f, 2.f)),
	std::make_shared<TweakVector>("cam.focus", &g_defaultFocus),
	std::make_shared<TweakVector>("cam.up", &g_defaultUp, vec3(0,0,1)),
	std::make_shared<TweakVector>("cam.orbitFocus", &g_orbitFocus),
	std::make_shared<TweakVector>("cam.orbitStart", &g_orbitStart, vec3(100,100,100)),
	std::make_shared<TweakFloat>("cam.orbitRate", &g_orbitRate, M_PI/180.f),
	std::make_shared<TweakFloat>("cam.orbitLength", &g_orbitLength, 1000.f),
	std::make_shared<TweakVector>("lighting.sundir", &g_sundir, vec3(0,0,1)),
	std::make_shared<TweakColor>("lighting.suncolor", &g_sunColor, Color(1,1,1)),
};

static void SaveCurrentCamera()
{
	g_defaultEye = g_mainCamera->GetPos();
	g_defaultFocus = g_mainCamera->GetPos() + g_mainCamera->GetViewframe().m_fwd;
	g_defaultUp = Normalize(g_mainCamera->GetViewframe().m_up);
}

////////////////////////////////////////////////////////////////////////////////
// Settings vars - don't get checked in to git
static std::vector<std::shared_ptr<TweakVarBase>> g_settingsVars = {
	std::make_shared<TweakBool>("cam.orbit", &g_orbitCam, false),
	std::make_shared<TweakBool>("debug.wireframe", &g_wireframe, false),
	std::make_shared<TweakBool>("debug.fpsDisplay", &g_debugDisplay, false),
	std::make_shared<TweakBool>("debug.draw", 
			[](){ return dbgdraw_IsEnabled(); },
			[](bool enabled) { dbgdraw_SetEnabled(int(enabled)); },
			false),
	std::make_shared<TweakBool>("debug.drawdepth", 
			[](){ return dbgdraw_IsDepthTestEnabled(); },
			[](bool enabled) { dbgdraw_SetDepthTestEnabled(int(enabled)); },
			true),

	std::make_shared<TweakInt>("record.fps", &g_recordFps, 30),
	std::make_shared<TweakInt>("record.count", &g_recordFrameCount, 300),
	std::make_shared<TweakFloat>("record.timeStart", &g_recordTimeRange.m_min, 1.0),
	std::make_shared<TweakFloat>("record.timeEnd", &g_recordTimeRange.m_max, 2.0),
};

////////////////////////////////////////////////////////////////////////////////
// Camera swap
static bool camera_GetDebugCamera()
{
	return (g_curCamera == g_debugCamera);
}

static void camera_SetDebugCamera(bool useDebug)
{
	if(useDebug) {
		g_curCamera = g_debugCamera;
	} else {
		g_curCamera = g_mainCamera;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Menu creation
static std::shared_ptr<TopMenuItem> MakeMenu()
{
	std::vector<std::shared_ptr<MenuItem>> cameraMenu = {
		std::make_shared<VecSliderMenuItem>("eye", &g_defaultEye),
		std::make_shared<VecSliderMenuItem>("focus", &g_defaultFocus),
		std::make_shared<VecSliderMenuItem>("orbit focus", &g_orbitFocus),
		std::make_shared<VecSliderMenuItem>("orbit start", &g_orbitStart),
		std::make_shared<FloatSliderMenuItem>("orbit rate", &g_orbitRate),
		std::make_shared<FloatSliderMenuItem>("orbit length", &g_orbitLength),
		std::make_shared<BoolMenuItem>("toggle orbit cam", &g_orbitCam),
		std::make_shared<ButtonMenuItem>("save current camera", SaveCurrentCamera)
	};
	std::vector<std::shared_ptr<MenuItem>> lightingMenu = {
		std::make_shared<VecSliderMenuItem>("sundir", &g_sundir),
		std::make_shared<ColorSliderMenuItem>("suncolor", &g_sunColor),
	};
	std::vector<std::shared_ptr<MenuItem>> debugMenu = {
		std::make_shared<ButtonMenuItem>("reload shaders", render_RefreshShaders),
		std::make_shared<BoolMenuItem>("wireframe", &g_wireframe),
		std::make_shared<BoolMenuItem>("fps & info", &g_debugDisplay),
		std::make_shared<BoolMenuItem>("debugcam", camera_GetDebugCamera, camera_SetDebugCamera),
		std::make_shared<IntSliderMenuItem>("debug texture id", 
			[&g_debugTexture](){return int(g_debugTexture);},
			[&g_debugTexture](int val) { g_debugTexture = static_cast<unsigned int>(val); }),
		std::make_shared<BoolMenuItem>("debug rendering", 
			[](){ return dbgdraw_IsEnabled(); },
			[](bool enabled) { dbgdraw_SetEnabled(int(enabled)); }),
		std::make_shared<BoolMenuItem>("debug depth test", 
			[](){ return dbgdraw_IsDepthTestEnabled(); },
			[](bool enabled) { dbgdraw_SetDepthTestEnabled(int(enabled)); }),
	};
	g_shapesMenu = std::make_shared<SubmenuMenuItem>("shapes");
	std::vector<std::shared_ptr<MenuItem>> tweakMenu = {
		std::make_shared<SubmenuMenuItem>("cam", std::move(cameraMenu)),
		std::make_shared<SubmenuMenuItem>("lighting", std::move(lightingMenu)),
		g_shapesMenu,
		std::make_shared<SubmenuMenuItem>("debug", std::move(debugMenu)),
	};
	std::vector<std::shared_ptr<MenuItem>> recordMenu = {
		std::make_shared<ButtonMenuItem>("take screenshot", [](){ g_screenshotRequested = true; }),
		std::make_shared<IntSliderMenuItem>("record fps", &g_recordFps),
		std::make_shared<IntSliderMenuItem>("record frame count", &g_recordFrameCount),
		std::make_shared<FloatSliderMenuItem>("start time", &g_recordTimeRange.m_min),
		std::make_shared<FloatSliderMenuItem>("end time", &g_recordTimeRange.m_max),
		std::make_shared<ButtonMenuItem>("start", record_Start),
	};
	std::vector<std::shared_ptr<MenuItem>> topMenu = {
		std::make_shared<SubmenuMenuItem>("tweak", std::move(tweakMenu)),
		std::make_shared<SubmenuMenuItem>("record", std::move(recordMenu)),
	};
	return std::make_shared<TopMenuItem>(topMenu);
}

////////////////////////////////////////////////////////////////////////////////
static void record_Start()
{
	if(g_recording) return;

	g_recordCurFrame = 0;
	g_dt = 1.f / g_recordFps;
	g_recording = true;
}

static void record_SaveFrame()
{
	ASSERT(g_recording);

	mkdir("frames", S_IRUSR | S_IWUSR | S_IXUSR);
	std::stringstream sstr ;
	sstr << "frames/frame_" << g_recordCurFrame << ".tga" ;
	std::cout << "Saving frame " << sstr.str() << std::endl;
	render_SaveScreen(sstr.str().c_str());
}

static void record_Advance()
{
	ASSERT(g_recording);

	++g_recordCurFrame;
	if(g_recordCurFrame >= g_recordFrameCount)
		g_recording = false;
	std::cout << "done." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
static void drawGround(const vec3& sundir)
{
	mat4 projview = g_curCamera->GetProj() * g_curCamera->GetView();
	mat4 model = MakeTranslation(0,0,-100) * MakeScale(vec3(500));
	mat4 modelIT = TransposeOfInverse(model);
	mat4 mvp = projview * model;

	const ShaderInfo* shader = g_groundShader.get();
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	GLint modelLoc = shader->m_uniforms[BIND_Model];
	GLint modelITLoc = shader->m_uniforms[BIND_ModelIT];
	GLint sundirLoc = shader->m_uniforms[BIND_Sundir];
	GLint sunColorLoc = shader->m_uniforms[BIND_SunColor];
	GLint eyePosLoc = shader->m_uniforms[BIND_Eyepos];
	GLint matShadowLoc = shader->m_custom[GRNDBIND_ShadowMatrix];
	GLint shadowMapLoc = shader->m_custom[GRNDBIND_ShadowMap];

	glUseProgram(shader->m_program);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_curHtex ? g_curHtex->m_gpuhtex->GetShadowTexture() : 0);
	glUniform1i(shadowMapLoc, 0);
	
	mat4 htexShadowMat = g_curHtex ? g_curHtex->m_gpuhtex->GetShadowMatrix() : (mat4::identity_t());
	mat4 matShadow = 
		MakeCoordinateScale(0.5f, 0.5f) *
		htexShadowMat * model;

	glUniformMatrix4fv(matShadowLoc, 1, 0, matShadow.m);
	glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);
	glUniformMatrix4fv(modelLoc, 1, 0, model.m);
	glUniformMatrix4fv(modelITLoc, 1, 0, modelIT.m);
	glUniform3fv(sundirLoc, 1, &sundir.x);
	glUniform3fv(sunColorLoc, 1, &g_sunColor.r);
	glUniform3fv(eyePosLoc, 1, &g_curCamera->GetPos().x);

	g_groundGeom->Render(*shader);
}

////////////////////////////////////////////////////////////////////////////////
static void draw(Framedata& frame)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.0f,0.0f,0.0f,1.f);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	int scissorHeight = g_screen.m_width/1.777;
	glScissor(0, 0.5*(g_screen.m_height - scissorHeight), g_screen.m_width, scissorHeight);
	if(!camera_GetDebugCamera()) {
		glEnable(GL_SCISSOR_TEST);
	}

	if(g_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	vec3 normalizedSundir = Normalize(g_sundir);

	////////////////////////////////////////////////////////////////////////////////
	if(!camera_GetDebugCamera()) glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	// ground render
	drawGround(normalizedSundir);

	// voxel render
	if(g_curHtex) 
		g_curHtex->m_gpuhtex->Render(*g_curCamera, normalizedSundir, g_sunColor);

	// everything below here is feedback for the user, so record the frame if we're recording
	if(g_recording)
	{
		record_SaveFrame();
		record_Advance();
	}

	// debug draw
	dbgdraw_Render(*g_curCamera);
	checkGlError("draw(): post dbgdraw");

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	render_drawDebugTexture(g_debugTexture, g_debugTextureSplit);

	if(g_menuEnabled)
		menu_Draw(*g_curCamera);

	checkGlError("draw(): post menu");

	if(g_debugDisplay)
	{
		char fpsStr[32] = {};
		static Color fpsCol = {1,1,1};
		snprintf(fpsStr, sizeof(fpsStr) - 1, "%.2f", g_fpsDisplay);
		font_Print(g_screen.m_width-180,24, fpsStr, fpsCol, 16.f);

		char cameraPosStr[64] = {};
		const vec3& pos = g_curCamera->GetPos();
		snprintf(cameraPosStr, sizeof(cameraPosStr) - 1, "eye: %.2f %.2f %.2f", pos.x, pos.y, pos.z);
		font_Print(g_screen.m_width-180, 40, cameraPosStr, fpsCol, 16.f);
	}

	task_RenderProgress();
	checkGlError("end draw");

	if(g_screenshotRequested) {
		render_SaveScreen("screenshot.tga");
		g_screenshotRequested = false;
	}
	
	SDL_GL_SwapBuffers();
	checkGlError("swap");
	
}

////////////////////////////////////////////////////////////////////////////////
static void createGpuHypertextures()
{
	g_htexList = ParseHtexFile("volumes.txt");
	
	// start the menu with an update button
	g_shapesMenu->AppendChild(std::make_shared<ButtonMenuItem>("update current", 
		[&g_curHtex]() 
		{
			if(g_curHtex) 
				g_curHtex->Update(Normalize(g_sundir)); 
		}));

	g_shapesMenu->AppendChild(std::make_shared<ButtonMenuItem>("save all",
		[]() { SaveHtexFile("volumes.txt", g_htexList); }));

	if(!g_htexList.empty())
	{
		g_curHtex = g_htexList[0];
		g_curHtex->Create();
		g_curHtex->Update(Normalize(g_sundir));
	}

	for(auto htex: g_htexList)
	{
		auto htexMenu = htex->CreateMenu();
		htexMenu->InsertChild(0,
			std::make_shared<ButtonMenuItem>("activate", 
				[htex, &g_curHtex]() { 
					if(g_curHtex) {
						g_curHtex->Destroy();
					}
					g_curHtex = htex; 
					g_curHtex->Create();
					g_curHtex->Update(Normalize(g_sundir));
				}));
		g_shapesMenu->AppendChild(htexMenu);
	}
}

////////////////////////////////////////////////////////////////////////////////	
static void initialize()
{
	task_Startup(3);
	dbgdraw_Init();
	render_Init();
	framemem_Init(); 
	font_Init();
	menu_SetTop(MakeMenu());
	ui_Init();
	hyper_Init();
	htexdb_Init();

	g_groundGeom = render_GeneratePlaneGeom();
	g_groundShader = render_CompileShader("shaders/ground.glsl", g_groundUniforms);

	g_mainCamera = std::make_shared<Camera>(30.f, g_screen.m_aspect);
	g_debugCamera = std::make_shared<Camera>(30.f, g_screen.m_aspect);

	tweaker_LoadVars("tweaker.txt", g_tweakVars);
	g_mainCamera->LookAt(g_defaultFocus, g_defaultEye, Normalize(g_defaultUp));
	g_curCamera = g_mainCamera;

	tweaker_LoadVars(".settings", g_settingsVars);
	
	createGpuHypertextures();
}

////////////////////////////////////////////////////////////////////////////////
static void orbitcam_Update()
{
	if(!g_orbitCam) return;
		
	static const vec3 kUp(0,0,1);
	vec3 startPos = g_orbitStart;
	mat4 rotZ = RotateAround(kUp, g_orbitAngle);
	vec3 pos = TransformPoint(rotZ, startPos);

	pos *= g_orbitLength / Length(pos);

	g_mainCamera->LookAt(g_orbitFocus, pos, kUp);

	g_orbitAngle += g_dt * g_orbitRate;
	g_orbitAngle = AngleWrap(g_orbitAngle);
}

static void updateFps()
{
	int numFrames = g_frameCount - g_frameSampleCount;
	if(numFrames >= 30 * 5)
	{
		g_frameTimer.Stop();
		
		float delta = g_frameTimer.GetTime();
		float fps = (numFrames) / delta;
		g_frameSampleCount = g_frameCount;
		g_fpsDisplay = fps;

		g_frameTimer.Start();
	}
}

////////////////////////////////////////////////////////////////////////////////
static void update(Framedata& frame)
{
	if(!g_recording)
	{
		constexpr float kMinDt = 1.0f/60.f;
		g_timer.Step(kMinDt);
		g_dt = g_timer.GetDt();
	}
	else
	{
		if(g_curHtex && g_recordTimeRange.Valid())
		{
			float time = g_recordTimeRange.Interpolate(g_recordCurFrame / (float)g_recordFrameCount);
			g_curHtex->m_time = time;
			g_curHtex->Update(Normalize(g_sundir));
		}
		g_dt = 1.0 / g_recordFps;
	}

	orbitcam_Update();

	updateFps();

	g_curCamera->Compute();

	menu_Update(g_dt);		

	gputask_Join();
	gputask_Kick();

	task_Update();
}

////////////////////////////////////////////////////////////////////////////////
enum CameraDirType {
	CAMDIR_FWD, 
	CAMDIR_BACK,
	CAMDIR_LEFT, 
	CAMDIR_RIGHT
};

static void move_camera(int dir)
{	
	int mods = SDL_GetModState();
	float kScale = 50.0;
	if(mods & KMOD_SHIFT && mods & KMOD_CTRL) kScale *= 100.f;
	else if (mods & KMOD_SHIFT) kScale *= 10.0f;

	vec3 dirs[] = {
		g_curCamera->GetViewframe().m_fwd,
		g_curCamera->GetViewframe().m_side,
	};

	vec3 off = dirs[dir>>1] * kScale * g_dt;
	if(1&dir) off = -off;

	g_curCamera->MoveBy(off);
}

////////////////////////////////////////////////////////////////////////////////
static void resize(int w, int h)
{
	g_screen.Resize(w,h);
	g_mainCamera->SetAspect(g_screen.m_aspect);
	g_debugCamera->SetAspect(g_screen.m_aspect);
	g_mainCamera->ComputeViewFrustum();
	g_debugCamera->ComputeViewFrustum();
	glViewport(0,0,w,h);
}

////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	SDL_Event event;

	SDL_SetVideoMode(g_screen.m_width, g_screen.m_height, 0, SDL_OPENGL | SDL_RESIZABLE);
	glewInit();
	glViewport(0,0,g_screen.m_width, g_screen.m_height);

	//SDL_ShowCursor(g_menuEnabled ? SDL_ENABLE : SDL_DISABLE );
	SDL_ShowCursor(SDL_ENABLE);

	glEnable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_3D);

	initialize();
	g_frameTimer.Start();

	checkGlError("after init");
	int done = 0;
	float keyRepeatTimer = 0.f;
	float nextKeyTimer = 0.f;
	int turning = 0, rolling = 0;
	int xTurnCenter = 0, yTurnCenter = 0;
	do
	{
		if(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_KEYDOWN:
					{
						keyRepeatTimer = 0.f;
						nextKeyTimer = 0.f;
						if(g_menuEnabled)
						{
							if(event.key.keysym.sym == SDLK_SPACE)
								g_menuEnabled = 1 ^ g_menuEnabled;
							else
								menu_Key(event.key.keysym.sym, event.key.keysym.mod);
						}
						else
						{
							switch(event.key.keysym.sym)
							{
								case SDLK_ESCAPE:
									done = 1;
									break;
								case SDLK_SPACE:
									g_menuEnabled = 1 ^ g_menuEnabled;
									SDL_ShowCursor(g_menuEnabled ? SDL_ENABLE : SDL_DISABLE );
									break;
								case SDLK_KP_PLUS:
									++g_debugTexture;
									break;
								case SDLK_KP_MINUS:
									if(g_debugTexture>0)
										--g_debugTexture;
									break;
								case SDLK_KP_ENTER:
									g_debugTextureSplit = !g_debugTextureSplit;
									break;
								case SDLK_PAGEUP:
									{
										vec3 off = 10.f * g_curCamera->GetViewframe().m_up ;
										g_curCamera->MoveBy(off);
									}
									break;
								case SDLK_PAGEDOWN:
									{
										vec3 off = -10.f * g_curCamera->GetViewframe().m_up;
										g_curCamera->MoveBy(off);
									}
									break;
								case SDLK_p:
									if(event.key.keysym.mod & KMOD_SHIFT)
										g_screenshotRequested = true;
									break;

								default: break;
							}
						}
					}
					break;

				case SDL_MOUSEBUTTONDOWN:
					{
						int xPos, yPos;
						Uint8 buttonState = SDL_GetMouseState(&xPos, &yPos);
						turning = (buttonState & SDL_BUTTON(SDL_BUTTON_LEFT));
						rolling = (buttonState & SDL_BUTTON(SDL_BUTTON_RIGHT));
						if(turning || rolling) {
							xTurnCenter = xPos; yTurnCenter = yPos;
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					{
						int xPos, yPos;
						Uint8 buttonState = SDL_GetMouseState(&xPos, &yPos);
						turning = (buttonState & SDL_BUTTON(SDL_BUTTON_LEFT));
						rolling = (buttonState & SDL_BUTTON(SDL_BUTTON_RIGHT));
					}
					break;
				case SDL_KEYUP:
					{
						keyRepeatTimer = 0.f;
						nextKeyTimer = 0.f;
					}
					break;
	
		
				case SDL_VIDEORESIZE:
				{
					int w = Max(event.resize.w, 256);
					int h = Max(event.resize.h, 256);
					SDL_Surface* surface = SDL_SetVideoMode(w, h, 0,
						SDL_OPENGL | SDL_RESIZABLE);
					if(!surface) {
						std::cerr << "Failed to resize window." << std::endl;
						abort();
					}
					resize(event.resize.w, event.resize.h);
				}
					
					break;
				case SDL_QUIT:
					done = 1;
					break;
				default: break;
			}
		}
	
		if(turning || rolling)
		{
			int xPos, yPos;
			(void)SDL_GetMouseState(&xPos, &yPos);
			int xDelta = xTurnCenter - xPos;
			int yDelta = yTurnCenter - yPos;
			if(turning)
			{
				float turn = (xDelta / g_screen.m_width) * (1 / 180.f) * M_PI;
				float tilt = -(yDelta / g_screen.m_height) * (1 / 180.f) * M_PI;
				g_curCamera->TurnBy(turn);
				g_curCamera->TiltBy(tilt);
			}

			if(rolling)
			{	
				float roll = -(xDelta / g_screen.m_width) * (1 / 180.f) * M_PI;
				g_curCamera->RollBy(roll);
			}
		}

		if(!g_menuEnabled)
		{
			const Uint8* keystate = SDL_GetKeyState(NULL);
			if(keystate[SDLK_w])
				move_camera(CAMDIR_FWD);
			if(keystate[SDLK_a])
				move_camera(CAMDIR_LEFT);
			if(keystate[SDLK_s])
				move_camera(CAMDIR_BACK);
			if(keystate[SDLK_d])
				move_camera(CAMDIR_RIGHT);

			if(g_curHtex)
			{
				float prevtime = g_curHtex->m_time;
				if(keystate[SDLK_UP])
					g_curHtex->m_time += g_dt;
				if(keystate[SDLK_DOWN])
					g_curHtex->m_time -= g_dt;

				if(prevtime != g_curHtex->m_time) {
					g_curHtex->Update(Normalize(g_sundir));
				}
			}
		}

		// clear old frame scratch space and recreate it.
		dbgdraw_Clear();
		framemem_Clear();
		Framedata* frame = frame_New();

		update(*frame);
		draw(*frame);
		g_frameCount++;

		keyRepeatTimer += g_dt;
		if(g_menuEnabled && keyRepeatTimer > 0.33f)
		{
			nextKeyTimer += g_dt;
			if(nextKeyTimer > 0.1f)
			{
				int mods = SDL_GetModState();
				const Uint8* keystate = SDL_GetKeyState(NULL);
				for(int i = 0; i < SDLK_LAST; ++i)
				{
					if(keystate[i])
						menu_Key(i, mods);
				}
				nextKeyTimer = 0.f;
			}
		}


	} while(!done);

	task_Shutdown();

	tweaker_SaveVars("tweaker.txt", g_tweakVars);
	tweaker_SaveVars(".settings", g_settingsVars);

	SDL_Quit();

	return 0;
}

