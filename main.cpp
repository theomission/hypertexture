#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <ctime>
#include <memory>
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
#include "hyper.hh"

////////////////////////////////////////////////////////////////////////////////
// globals

Screen g_screen(1024, 768);
std::shared_ptr<Camera> g_curCamera;

////////////////////////////////////////////////////////////////////////////////
// file scope globals
static float g_dt;
static vec3 g_focus, g_defaultFocus, g_defaultEye, g_defaultUp;
static std::shared_ptr<Camera> g_mainCamera;
static std::shared_ptr<Camera> g_debugCamera;

static int g_menuEnabled = 0;
static int g_wireframe = 0;

static float g_fpsDisplay = 0.f;
static vec3 g_fakeFocus = {-10.f, 0.f, 0.f};
static Viewframe g_debugViewframe;

static int g_frameCount;
static int g_frameSampleCount;
static unsigned long long g_frameSampleTime;
static unsigned long long g_lastTime = 0;

static std::shared_ptr<Hypertexture> g_htex;

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<ShaderInfo> g_debugTexShader;
enum DebugTexUniformLocType {
	DTEXLOC_Tex1D,
	DTEXLOC_Tex2D,
	DTEXLOC_Channel,
	DTEXLOC_Dims,
	DTEXLOC_NUM,
};

static std::vector<CustomShaderAttr> g_debugTexUniformNames = 
{
	{ DTEXLOC_Tex1D, "colorTex1d" },
	{ DTEXLOC_Tex2D, "colorTex2d" },
	{ DTEXLOC_Channel, "channel" },
	{ DTEXLOC_Dims, "dims"},
};

////////////////////////////////////////////////////////////////////////////////
static GLuint g_debugTexture;
static int g_debugTextureSplit;

////////////////////////////////////////////////////////////////////////////////
// tweak vars
extern float g_tileDrawErrorThreshold;
static std::vector<std::shared_ptr<TweakVarBase>> g_tweakVars = {
	std::make_shared<TweakVector>("cam.eye", &g_defaultEye, vec3{8.f, 0.f, 2.f}),
	std::make_shared<TweakVector>("cam.focus", &g_defaultFocus),
	std::make_shared<TweakVector>("cam.up", &g_defaultUp, vec3{0,0,1}),
};

void SaveCurrentCamera()
{
	g_defaultEye = g_mainCamera->GetPos();
	g_defaultFocus = g_mainCamera->GetPos() + g_mainCamera->GetViewframe().m_fwd;
	g_defaultUp = Normalize(g_mainCamera->GetViewframe().m_up);
}

////////////////////////////////////////////////////////////////////////////////
// Settings vars
static std::vector<std::shared_ptr<TweakVarBase>> g_settingsVars = {
	std::make_shared<TweakBool>("debug.wireframe", &g_wireframe, false),
	std::make_shared<TweakBool>("debug.draw", 
			[](){ return dbgdraw_IsEnabled(); },
			[](bool enabled) { dbgdraw_SetEnabled(int(enabled)); },
			false),
	std::make_shared<TweakBool>("debug.drawdepth", 
			[](){ return dbgdraw_IsDepthTestEnabled(); },
			[](bool enabled) { dbgdraw_SetDepthTestEnabled(int(enabled)); },
			true),
};

////////////////////////////////////////////////////////////////////////////////
// Camera swap
bool camera_GetDebugCamera()
{
	return (g_curCamera == g_debugCamera);
}

void camera_SetDebugCamera(bool useDebug)
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
		std::make_shared<ButtonMenuItem>("save current camera", SaveCurrentCamera)
	};
	std::vector<std::shared_ptr<MenuItem>> debugMenu = {
		std::make_shared<ButtonMenuItem>("reload shaders", render_RefreshShaders),
		std::make_shared<BoolMenuItem>("wireframe", &g_wireframe),
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
	std::vector<std::shared_ptr<MenuItem>> tweakMenu = {
		std::make_shared<SubmenuMenuItem>("cam", cameraMenu),
		std::make_shared<SubmenuMenuItem>("debug", debugMenu),
	};
	std::vector<std::shared_ptr<MenuItem>> topMenu = {
		std::make_shared<SubmenuMenuItem>("tweak", tweakMenu)
	};
	return std::make_shared<TopMenuItem>(topMenu);
}

////////////////////////////////////////////////////////////////////////////////
void drawDebugTexture(void)
{
	if(g_debugTexture)
	{	
		GLint cur;
		float x = 20, y = 20, w = 40, h = 350, scale = 1.f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, g_debugTexture); 
		glBindTexture(GL_TEXTURE_2D, 0);
		glGetIntegerv(GL_TEXTURE_BINDING_1D, &cur);
		bool is2d = (GLuint)cur != g_debugTexture;
		if(is2d)
		{
			(void)glGetError(); // clear the error so it doesn't spam
			glBindTexture(GL_TEXTURE_1D, 0);
			glBindTexture(GL_TEXTURE_2D, g_debugTexture);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		const ShaderInfo* shader = g_debugTexShader.get();
		GLuint program = shader->m_program;
		GLint posLoc = shader->m_attrs[GEOM_Pos];
		GLint uvLoc = shader->m_attrs[GEOM_Uv];
		GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
		GLint tex1dLoc = shader->m_custom[DTEXLOC_Tex1D];
		GLint tex2dLoc = shader->m_custom[DTEXLOC_Tex2D];
		GLint channelLoc = shader->m_custom[DTEXLOC_Channel];
		GLint dimsLoc = shader->m_custom[DTEXLOC_Dims];

		glUseProgram(program);
		glUniformMatrix4fv(mvpLoc, 1, 0, g_screen.m_proj.m);
		glUniform1i(is2d ? tex2dLoc : tex1dLoc, 0);
		glUniform1i(dimsLoc, is2d ? 2 : 1);

		if(!is2d) h = 710;
		int channel = g_debugTextureSplit ? 0 : 4;
		int channelMax = g_debugTextureSplit ? 4 : 5;
		for(; channel < channelMax; ++channel)
		{
			glUniform1i(channelLoc, channel);
			if(!is2d) w = 40;
			else w = 350;

			if(!g_debugTextureSplit) { if(is2d) { w = 710; } h = 710; }

			glBegin(GL_TRIANGLE_STRIP);
			glVertexAttrib2f(uvLoc, 0, 0); glVertexAttrib3f(posLoc, x,y,0.f);
			glVertexAttrib2f(uvLoc, scale, 0); glVertexAttrib3f(posLoc, x+w,y,0.f);
			glVertexAttrib2f(uvLoc, 0, scale); glVertexAttrib3f(posLoc, x,y+h,0.f);
			glVertexAttrib2f(uvLoc, scale, scale); glVertexAttrib3f(posLoc, x+w,y+h,0.f);
			glEnd();

			if(!is2d)
			{
				x = x+w+10;
			}
			else
			{
				if((channel & 1) == 0)
				{
					x = x + w + 10;
				}
				else
				{
					x = 20;
					y = y + h + 10;
				}
			}
		}
		checkGlError("debug draw texture");
	}
}

////////////////////////////////////////////////////////////////////////////////
void draw(Framedata& frame)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.0f,0.0f,0.0f,1.f);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	int scissorHeight = g_screen.m_width/1.777;
	glScissor(0, 0.5*(g_screen.m_height - scissorHeight), g_screen.m_width, scissorHeight);
	if(!camera_GetDebugCamera()) glEnable(GL_SCISSOR_TEST);
	if(g_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	////////////////////////////////////////////////////////////////////////////////
	if(!camera_GetDebugCamera()) glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	g_htex->Render(*g_curCamera, 10.f);
	
	dbgdraw_Render(*g_curCamera);
	checkGlError("draw(): post dbgdraw");

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	drawDebugTexture();

	if(g_menuEnabled)
		menu_Draw();

	checkGlError("draw(): post menu");

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

	SDL_GL_SwapBuffers();
	checkGlError("swap");
}

void InitializeShaders()
{
	g_debugTexShader = render_CompileShader("shaders/debugtex2d.glsl", g_debugTexUniformNames);
	checkGlError("InitializeShaders");
}

void createHyperTexture()
{
	struct gendata {
		float invradius;
		vec3 center;
	};

	std::shared_ptr<gendata> data = std::make_shared<gendata>();
	data->invradius = 1.f/0.5f;
	data->center.Set(0.5f,1.0f,1.f);

	g_htex = std::make_shared<Hypertexture>(256,
	[=](float x, float y, float z)
	{
		vec3 pt(x,y,z);
		float len = Length(pt - data->center);
		float density = Max(1.f - len * data->invradius, 0.f);
		return density;
	});
}

void initialize()
{
	task_Startup(3);
	dbgdraw_Init();
	InitializeShaders();
	framemem_Init();
	font_Init();
	menu_SetTop(MakeMenu());
	ui_Init();
	hyper_Init();

	g_mainCamera = std::make_shared<Camera>(30.f, g_screen.m_aspect);
	g_debugCamera = std::make_shared<Camera>(30.f, g_screen.m_aspect);

	tweaker_LoadVars("tweaker.txt", g_tweakVars);
	g_mainCamera->LookAt(g_defaultFocus, g_defaultEye, Normalize(g_defaultUp));
	g_curCamera = g_mainCamera;

	tweaker_LoadVars(".settings", g_settingsVars);
	
	createHyperTexture();
}

void update(Framedata& frame)
{
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	unsigned long long timeUsec = (unsigned long long)(current_time.tv_sec * 1000000) +
		(unsigned long long)(current_time.tv_nsec / 1000);
	
	unsigned long long diffTime = timeUsec - g_lastTime;
	if(diffTime > 16000)
	{
		g_lastTime = timeUsec;
		float dt = diffTime / 1e6f;
		g_dt = dt;

		// move stuff with dt
	}
	else g_dt = 0.f;

	if(g_frameCount - g_frameSampleCount > 30 * 5)
	{
		unsigned long long diffTime = timeUsec - g_frameSampleTime;
		g_frameSampleTime = timeUsec;

		float delta = diffTime / 1e6f;
		float fps = (g_frameCount - g_frameSampleCount) / delta;
		g_frameSampleCount = g_frameCount;
		g_fpsDisplay = fps;
	}

	g_curCamera->Compute();

	menu_Update(g_dt);		

	gputask_Join();
	gputask_Kick();

	task_Update();
}

enum CameraDirType {
	CAMDIR_FWD, 
	CAMDIR_BACK,
	CAMDIR_LEFT, 
	CAMDIR_RIGHT
};

void move_camera(int dir)
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

void resize(int w, int h)
{
	g_screen.Resize(w,h);
	g_mainCamera->SetAspect(g_screen.m_aspect);
	g_debugCamera->SetAspect(g_screen.m_aspect);
	g_mainCamera->ComputeViewFrustum();
	g_debugCamera->ComputeViewFrustum();
	glViewport(0,0,w,h);
}

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
									g_debugTextureSplit = 1 ^ g_debugTextureSplit;
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
				int i;
				for(i = 0; i < SDLK_LAST; ++i)
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

