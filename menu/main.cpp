
#include <sal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>
#include "conffile.h"
#include "display.h"
#include "controls.h"
#include "unzip.h"
#include "zip.h"
#include "menu.h"
#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "scaler.h"

#define SNES_SCREEN_WIDTH  256
#define SNES_SCREEN_HEIGHT 192

static struct MENU_OPTIONS mMenuOptions;
static int mEmuScreenHeight;
static int mEmuScreenWidth;
static char mRomName[SAL_MAX_PATH]={""};
static u32 mLastRate=0;

static s8 mFpsDisplay[16]={""};
static s8 mVolumeDisplay[16]={""};
static s8 mQuickStateDisplay[16]={""};
static u32 mFps=0;
static u32 mLastTimer=0;
static u32 mEnterMenu=0;
static u32 mLoadRequested=0;
static u32 mSaveRequested=0;
static u32 mQuickStateTimer=0;
static u32 mVolumeTimer=0;
static u32 mVolumeDisplayTimer=0;
static u32 mFramesCleared=0;
static u32 mInMenu=0;

static const uint32 P1_L = 1;
static const uint32 P1_R = 2;
static const uint32 P1_Up = 3;
static const uint32 P1_Down = 4;
static const uint32 P1_Left = 5;
static const uint32 P1_Right = 6;
static const uint32 P1_Select = 7;
static const uint32 P1_Start = 8;
static const uint32 P1_A = 9;
static const uint32 P1_B = 10;
static const uint32 P1_X = 11;
static const uint32 P1_Y = 12;

static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}

void S9xProcessSound (unsigned int)
{
}

void S9xExit ()
{
}

void S9xSetPalette ()
{

}

void S9xExtraUsage ()
{
}
	
void S9xParseArg (char **argv, int &index, int argc)
{	
}

bool8 S9xOpenSnapshotFile (const char *fname, bool8 read_only, STREAM *file)
{
	if (read_only)
	{
		if ((*file = OPEN_STREAM(fname,"rb")))
			return(TRUE);
	}
	else
	{
#ifdef ZLIB
		if ((*file = OPEN_STREAM(fname,"wb1h"))) /* level 1, Huffman only */
			return(TRUE);
#else
		if ((*file = OPEN_STREAM(fname,"wb")))
			return(TRUE);
#endif
	}

	return (FALSE);	
}
	
void S9xCloseSnapshotFile (STREAM file)
{
	CLOSE_STREAM(file);
}

void S9xMessage (int /* type */, int /* number */, const char *message)
{
	//MenuMessageBox("PocketSnes has encountered an error",(s8*)message,"",MENU_MESSAGE_BOX_MODE_PAUSE);
}

u16 IntermediateScreen[SNES_WIDTH * SNES_HEIGHT_EXTENDED];

bool8 S9xInitUpdate ()
{
	if (mInMenu) return TRUE;

	GFX.Screen = IntermediateScreen; /* replacement needed after loading the saved states menu */
	return TRUE;
}

bool8 S9xContinueUpdate (int Width, int Height)
{
}

bool8 S9xDeinitUpdate (int Width, int Height)
{
	if(mInMenu) return TRUE;

	// After returning from the menu, clear the background of 3 frames.
	// This prevents remnants of the menu from appearing.
	if (mFramesCleared < 3)
	{
		sal_VideoClear(0);
		mFramesCleared++;
	}

	if (mMenuOptions.fullScreen == 1)
	{
		upscale_p((uint32_t*) sal_VideoGetBuffer(), (uint32_t*) IntermediateScreen, SNES_WIDTH);
	}
	if (mMenuOptions.fullScreen == 2)
	{
		upscale_256x224_to_320x240_bilinearish((uint32_t*) sal_VideoGetBuffer() + 160, (uint32_t*) IntermediateScreen, SNES_WIDTH);
	}
	if (mMenuOptions.fullScreen == 0)
	{
		u32 y, pitch = sal_VideoGetPitch();
		u8 *src = (u8*) IntermediateScreen, *dst = (u8*) sal_VideoGetBuffer() + ((SAL_SCREEN_WIDTH - SNES_WIDTH) / 2 + (((SAL_SCREEN_HEIGHT - SNES_HEIGHT) / 2) * SAL_SCREEN_WIDTH)) * sizeof(u16);
		for (y = 0; y < SNES_HEIGHT; y++)
		{
			memcpy(dst, src, SNES_WIDTH * sizeof(u16));
			src += SNES_WIDTH * sizeof(u16);
			dst += pitch;
		}
	}

	u32 newTimer;
	if (mMenuOptions.showFps) 
	{
		mFps++;
		newTimer=sal_TimerRead();
		if(newTimer-mLastTimer>Memory.ROMFramesPerSecond)
		{
			mLastTimer=newTimer;
			sprintf(mFpsDisplay,"FPS: %d / %d", mFps, Memory.ROMFramesPerSecond);
			mFps=0;
		}
		
		sal_VideoDrawRect(0,0,13*8,8,SAL_RGB(0,0,0));
		sal_VideoPrint(0,0,mFpsDisplay,SAL_RGB(31,31,31));
	}

	if(mVolumeDisplayTimer>0)
	{
		sal_VideoDrawRect(100,0,8*8,8,SAL_RGB(0,0,0));
		sal_VideoPrint(100,0,mVolumeDisplay,SAL_RGB(31,31,31));
	}

	if(mQuickStateTimer>0)
	{
		sal_VideoDrawRect(200,0,8*8,8,SAL_RGB(0,0,0));
		sal_VideoPrint(200,0,mQuickStateDisplay,SAL_RGB(31,31,31));
	}

	sal_VideoFlip(0);
}

/* _splitpath/_makepath: Modified from unix.cpp. See file for credits. */
#ifndef SLASH_CHAR
#define SLASH_CHAR '/'
#endif

void
_splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	char *slash = strrchr ((char *) path, SLASH_CHAR);
	char *dot   = strrchr ((char *) path, '.');

	*drive = '\0';

	if (dot && slash && dot < slash)
	{
		dot = 0;
	}

	if (!slash)
	{
		*dir = '\0';
		strcpy (fname, path);

		if (dot)
		{
			fname[dot - path] = '\0';
			strcpy (ext, dot + 1);
		}
		else
		{
			*ext = '\0';
		}
	}
	else
	{
		strcpy (dir, path);
		dir[slash - path] = '\0';
		strcpy (fname, slash + 1);

		if (dot)
		{
			fname[(dot - slash) - 1] = '\0';
			strcpy (ext, dot + 1);
		}
		else
		{
			*ext = '\0';
		}
	}
}

void
_makepath (char       *path,
           const char *drive,
           const char *dir,
           const char *fname,
           const char *ext)
{
	if (dir && *dir)
	{
		strcpy (path, dir);
		strcat (path, "/");
	}
	else
	*path = '\0';

	strcat (path, fname);

	if (ext && *ext)
	{
		strcat (path, ".");
		strcat (path, ext);
	}
}

const char *
S9xGetFilenameInc (const char *e, enum s9x_getdirtype dirtype)
{
	static char  filename[PATH_MAX + 1];
	char         dir[_MAX_DIR + 1];
	char         drive[_MAX_DRIVE + 1];
	char         fname[_MAX_FNAME + 1];
	char         ext[_MAX_EXT + 1];
	unsigned int i = 0;
	struct stat  buf;
	const char   *d;

	_splitpath (Memory.ROMFilename, drive, dir, fname, ext);
	d = S9xGetDirectory (dirtype);

	do
	{
		snprintf (filename, sizeof (filename),
			"%s" SLASH_STR "%s%03d%s", d, fname, i, e);
		i++;
	}
	while (stat (filename, &buf) == 0 && i != 0); /* Overflow? ...riiight :-) */

	return (filename);
}

const char *
S9xGetDirectory (enum s9x_getdirtype dirtype)
{
	static char path[PATH_MAX + 1];

	switch (dirtype)
	{
		case HOME_DIR:
			strcpy (path, sal_DirectoryGetUser());
			break;

		case SRAM_DIR:
		case SNAPSHOT_DIR:
			strcpy(path, sal_DirectoryGetHome());
			break;

		case SCREENSHOT_DIR:
		case SPC_DIR:
		case CHEAT_DIR:
		case IPS_DIR:
		default:
			path[0] = '\0';
	}

	/* Try and mkdir, whether it exists or not */
	if (dirtype != HOME_DIR && path[0] != '\0')
	{
		mkdir (path, 0755);
		chmod (path, 0755);
	}

	/* Anything else, use ROM filename path */
	if (path[0] == '\0')
	{
		char *loc;

		strcpy (path, Memory.ROMFilename);

		loc = strrchr (path, SLASH_CHAR);

		if (loc == NULL)
		{
			if (getcwd (path, PATH_MAX + 1) == NULL)
			{
				strcpy (path, getenv ("HOME"));
			}
		}
		else
		{
			path[loc - path] = '\0';
		}
	}

	return path;
}

const char *
S9xGetFilename (const char *ex, enum s9x_getdirtype dirtype)
{
	static char filename[PATH_MAX + 1];
	char        dir[_MAX_DIR + 1];
	char        drive[_MAX_DRIVE + 1];
	char        fname[_MAX_FNAME + 1];
	char        ext[_MAX_EXT + 1];

	_splitpath (Memory.ROMFilename, drive, dir, fname, ext);

	snprintf (filename, sizeof (filename), "%s" SLASH_STR "%s%s",
		S9xGetDirectory (dirtype), fname, ex);

	return (filename);
}

const char *
S9xBasename (const char *f)
{
    const char *p;

    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
        return (p + 1);

    return f;
}

void
S9xToggleSoundChannel (int c)
{
    static int sound_switch = 255;

    if (c == 8)
        sound_switch = 255;
    else
        sound_switch ^= 1 << c;

    S9xSetSoundControl (sound_switch);
}

const char* S9xChooseFilename(bool8 read_only)
{
	return NULL;
}

const char* S9xChooseMovieFilename(bool8 read_only)
{
	return NULL;
}

void S9xHandlePortCommand (s9xcommand_t cmd, int16 data1, int16 data2)
{
}

void S9xParsePortConfig(ConfigFile&, int pass)
{
}

void PSNESReadJoypad ()
{
	u32 joy = sal_InputPoll();
	
	if (joy & SAL_INPUT_MENU)
	{
		mEnterMenu = 1;		
		return;
	}

#if 0
	if ((joy & SAL_INPUT_L)&&(joy & SAL_INPUT_R)&&(joy & SAL_INPUT_UP))
	{
		if(mVolumeTimer==0)
		{
			mMenuOptions.volume++;
			if(mMenuOptions.volume>31) mMenuOptions.volume=31;
			sal_AudioSetVolume(mMenuOptions.volume,mMenuOptions.volume);
			mVolumeTimer=5;
			mVolumeDisplayTimer=60;
			sprintf(mVolumeDisplay,"Vol: %d",mMenuOptions.volume);
		}
		return;
	}

	if ((joy & SAL_INPUT_L)&&(joy & SAL_INPUT_R)&&(joy & SAL_INPUT_DOWN))
	{
		if(mVolumeTimer==0)
		{
			mMenuOptions.volume--;
			if(mMenuOptions.volume>31) mMenuOptions.volume=0;
			sal_AudioSetVolume(mMenuOptions.volume,mMenuOptions.volume);
			mVolumeTimer=5;
			mVolumeDisplayTimer=60;
			sprintf(mVolumeDisplay,"Vol: %d",mMenuOptions.volume);
		}
		return;
	}
#endif

	S9xReportButton(P1_L, (joy & SAL_INPUT_L) != 0);
	S9xReportButton(P1_R, (joy & SAL_INPUT_R) != 0);
	S9xReportButton(P1_Up, (joy & SAL_INPUT_UP) != 0);
	S9xReportButton(P1_Down, (joy & SAL_INPUT_DOWN) != 0);
	S9xReportButton(P1_Left, (joy & SAL_INPUT_LEFT) != 0);
	S9xReportButton(P1_Right, (joy & SAL_INPUT_RIGHT) != 0);
	S9xReportButton(P1_Select, (joy & SAL_INPUT_SELECT) != 0);
	S9xReportButton(P1_Start, (joy & SAL_INPUT_START) != 0);
	S9xReportButton(P1_A, (joy & SAL_INPUT_A) != 0);
	S9xReportButton(P1_B, (joy & SAL_INPUT_B) != 0);
	S9xReportButton(P1_X, (joy & SAL_INPUT_X) != 0);
	S9xReportButton(P1_Y, (joy & SAL_INPUT_Y) != 0);
}

bool S9xPollAxis (uint32 id, int16 *value)
{
	return false;
}

bool S9xPollPointer (uint32 id, int16 *x, int16 *y)
{
	return false;
}

bool S9xPollButton (uint32 id, bool *pressed)
{
	return false;
}

const char * S9xStringInput (const char *message)
{
	return NULL;
}

void S9xSyncSpeed(void)
{
 	S9xSyncSound();
}

void PSNESForceSaveSRAM (void)
{
	if(mRomName[0] != 0)
	{
		Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
	}
}

void PSNESSaveSRAM (int showWarning)
{
	if (CPU.SRAMModified)
	{
		if(!Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR)))
		{
			MenuMessageBox("Saving SRAM","Failed!","",MENU_MESSAGE_BOX_MODE_PAUSE);
		}
	}
	else if(showWarning)
	{
		MenuMessageBox("SRAM saving ignored","No changes have been made to SRAM","",MENU_MESSAGE_BOX_MODE_MSG);
	}
}

bool8 PSNESInitSound()
{
	return S9xInitSound(60, 20);
}

bool8 S9xOpenSoundDevice()
{
	return TRUE;
}

void S9xAutoSaveSRAM (void)
{
	if (mMenuOptions.autoSaveSram)
	{
		Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
		// sync(); // Only sync at exit or with a ROM change
	}
}

void S9xLoadSRAM (void)
{
	Memory.LoadSRAM (S9xGetFilename (".srm", SRAM_DIR));
}

static u32 LastAudioRate = 0;  // None yet, because audio is not open
static bool8 LastStereo = FALSE;

static
int Run(int sound)
{
  	int skip=0, done=0, doneLast=0,aim=0,i;
	sal_TimerInit(Settings.FrameTime);
	done=sal_TimerRead()-1;

	if (sound) {
		if (LastAudioRate != mMenuOptions.soundRate || LastStereo != mMenuOptions.stereo)
		{
			if (LastAudioRate != 0)
			{
				sal_AudioClose();
			}

			// DATA RACE AVOIDANCE
			// While the audio output is closed, there is no chance that the
			// audio stream will request samples from Snes9x. Update settings.
			Settings.SoundPlaybackRate = mMenuOptions.soundRate;
			Settings.Stereo = mMenuOptions.stereo ? TRUE : FALSE;
			if (!PSNESInitSound())
			{
				fprintf(stderr, "S9xInitSound failed after rate change from %" PRIu32 " to %" PRIu32 "; sound will be muted\n", LastAudioRate, Settings.SoundPlaybackRate);
			}
			LastAudioRate = Settings.SoundPlaybackRate;
			LastStereo = Settings.Stereo;

			sal_AudioInit(Settings.SoundPlaybackRate, Settings.SixteenBitSound ? 16 : 8,
						Settings.Stereo, Memory.ROMFramesPerSecond);
		}
		sal_AudioResume();
	}

  	while(!mEnterMenu) 
  	{
		for (i=0;i<10;i++)
		{
			aim=sal_TimerRead();
			if (done < aim)
			{
				done++;
				if (mMenuOptions.frameSkip == 0) //Auto
					IPPU.RenderThisFrame = (done >= aim);
				else if ((IPPU.RenderThisFrame = (--skip <= 0)))
					skip = mMenuOptions.frameSkip;

				//Run SNES for one glorious frame
				S9xMainLoop ();
				PSNESReadJoypad ();
			}
			if (done>=aim) break; // Up to date now
			if (mEnterMenu) break;
		}
		done=aim; // Make sure up to date
  	}

	if (sound)
	{
		sal_AudioPause();
	}

	mEnterMenu=0;
	return mEnterMenu;

}

static inline int RunSound(void)
{
	return Run(1);
}

static inline int RunNoSound(void)
{
	return Run(0);
}

static 
int SnesRomLoad()
{
	char filename[SAL_MAX_PATH+1];
	int check;
	char text[256];
	FILE *stream=NULL;
  
    	MenuMessageBox("Loading ROM...",mRomName,"",MENU_MESSAGE_BOX_MODE_MSG);

	if (!Memory.LoadROM (mRomName))
	{
		MenuMessageBox("Loading ROM",mRomName,"Failed!",MENU_MESSAGE_BOX_MODE_PAUSE);
		return SAL_ERROR;
	}
	
	MenuMessageBox("Done loading the ROM",mRomName,"",MENU_MESSAGE_BOX_MODE_MSG);

	S9xLoadSRAM();
	return SAL_OK;
}

int SnesInit()
{
	ZeroMemory (&Settings, sizeof (Settings));

	// Here are our timings.
	Settings.FrameTimeNTSC = 16667;
	Settings.FrameTimePAL = 20000;
	Settings.SkipFrames = AUTO_FRAMERATE;
	Settings.TurboSkipFrames = 5;
	Settings.AutoMaxSkipFrames = 5;
	Settings.TurboMode = FALSE;
	Settings.HighSpeedSeek = TRUE;
	Settings.FrameAdvance = FALSE;

	// Here are our emulation settings.
	Settings.BlockInvalidVRAMAccessMaster = TRUE;
	Settings.HDMATimingHack = 100;
	Settings.ApplyCheats = FALSE;
	Settings.NoPatch = FALSE;
	Settings.AutoSaveDelay = 1;
	Settings.DontSaveOopsSnapshot = TRUE;

	// Here's how we do sound, video and input.
	Settings.SoundSync = FALSE;
	Settings.SixteenBitSound = TRUE;
	Settings.SoundInputRate = 32000;
	Settings.ReverseStereo = FALSE;
	Settings.SupportHiRes = FALSE;
	GFX.Screen = (uint16*) malloc(SNES_WIDTH * SNES_HEIGHT_EXTENDED * sizeof(uint16));
	GFX.Pitch = SNES_WIDTH * sizeof(uint16);
	Settings.OpenGLEnable = FALSE;
	Settings.Transparency = TRUE;
	Settings.DisplayFrameRate = FALSE;
	Settings.DisplayWatchedAddresses = FALSE;
	Settings.DisplayPressedKeys = FALSE;
	Settings.DisplayMovieFrame = FALSE;
	Settings.AutoDisplayMessages = FALSE;
	Settings.InitialInfoStringTimeout = 120;
	Settings.UpAndDown = FALSE;

	S9xUnmapAllControls();
	S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
	S9xMapButton(P1_L, S9xGetCommandT("Joypad1 L"), false);
	S9xMapButton(P1_R, S9xGetCommandT("Joypad1 R"), false);
	S9xMapButton(P1_Up, S9xGetCommandT("Joypad1 Up"), false);
	S9xMapButton(P1_Down, S9xGetCommandT("Joypad1 Down"), false);
	S9xMapButton(P1_Left, S9xGetCommandT("Joypad1 Left"), false);
	S9xMapButton(P1_Right, S9xGetCommandT("Joypad1 Right"), false);
	S9xMapButton(P1_Select, S9xGetCommandT("Joypad1 Select"), false);
	S9xMapButton(P1_Start, S9xGetCommandT("Joypad1 Start"), false);
	S9xMapButton(P1_A, S9xGetCommandT("Joypad1 A"), false);
	S9xMapButton(P1_B, S9xGetCommandT("Joypad1 B"), false);
	S9xMapButton(P1_X, S9xGetCommandT("Joypad1 X"), false);
	S9xMapButton(P1_Y, S9xGetCommandT("Joypad1 Y"), false);

	// Disable all tracing.
	Settings.TraceDMA = FALSE;
	Settings.TraceHDMA = FALSE;
	Settings.TraceVRAM = FALSE;
	Settings.TraceUnknownRegisters = FALSE;
	Settings.TraceDSP = FALSE;
	Settings.TraceHCEvent = FALSE;

	// Allow these peripherals.
	Settings.MouseMaster = FALSE;
	Settings.SuperScopeMaster = FALSE;
	Settings.JustifierMaster = FALSE;
	Settings.MultiPlayer5Master = FALSE;

	// Multi-cartridge loading.
	Settings.Multi = FALSE;
	Settings.CartAName[0] = '\0';
	Settings.CartBName[0] = '\0';

	// Network play settings.
	Settings.NetPlay = FALSE;
	Settings.NetPlayServer = FALSE;
	Settings.ServerName[0] = '\0';
	Settings.Port = 6096;

	// Movie settings.
	Settings.MovieTruncate = FALSE;
	Settings.MovieNotifyIgnored = FALSE;
	Settings.WrongMovieStateProtection = TRUE;
	Settings.DumpStreams = FALSE;
	Settings.DumpStreamsMaxFrames = -1;

	// User settings.
	Settings.SoundPlaybackRate = 44100;
	Settings.Stereo = TRUE;

	if (!Memory.Init())
	{
		fprintf(stderr, "Snes9x main memory allocation error\n");
		return SAL_ERROR;
	}
	if (!S9xInitAPU())
	{
		fprintf(stderr, "Snes9x APU initialisation failed\n");
		goto cleanup_memory;
	}
	if (!S9xGraphicsInit())
	{
		fprintf(stderr, "Snes9x graphics initialisation failed\n");
		goto cleanup_apu;
	}
	if (!PSNESInitSound())
	{
		fprintf(stderr, "Snes9x sound memory allocation error\n");
		goto cleanup_graphics;
	}
	S9xSetSoundMute (FALSE);

	return SAL_OK;

cleanup_graphics:
	S9xGraphicsDeinit();
cleanup_apu:
	S9xDeinitAPU();
cleanup_memory:
	Memory.Deinit();
	return SAL_ERROR;
}

int mainEntry(int argc, char* argv[])
{
	int ref = 0;

	s32 event=EVENT_NONE;

	if (!sal_Init())
		return 1;
	if (!sal_VideoInit(16,SAL_RGB(0,0,0),Memory.ROMFramesPerSecond))
	{
		sal_Reset();
		return 1;
	}

	mRomName[0]=0;
	if (argc >= 2) 
 		strcpy(mRomName, argv[1]); // Record ROM name

	MenuInit(sal_DirectoryGetHome(), &mMenuOptions);


	if(SnesInit() == SAL_ERROR)
	{
		sal_Reset();
		return 1;
	}

	while(1)
	{
		mInMenu=1;
		event=MenuRun(mRomName);
		mInMenu=0;

		if(event==EVENT_LOAD_ROM)
		{
			if (mRomName[0] != 0)
			{
				MenuMessageBox("Saving SRAM...","","",MENU_MESSAGE_BOX_MODE_MSG);
				PSNESForceSaveSRAM();
			}
			if(SnesRomLoad() == SAL_ERROR) 
			{
				mRomName[0] = 0;
				MenuMessageBox("Failed to load ROM",mRomName,"Press any button to continue", MENU_MESSAGE_BOX_MODE_PAUSE);
				sal_Reset();
		    		return 0;
			}
			else
			{
				event=EVENT_RUN_ROM;
		  	}
		}

		if(event==EVENT_RESET_ROM)
		{
			S9xReset();
			event=EVENT_RUN_ROM;
		}

		if(event==EVENT_RUN_ROM)
		{
			if(mMenuOptions.fullScreen)
			{
				sal_VideoSetScaling(SNES_WIDTH,SNES_HEIGHT);
			}

			if(mMenuOptions.transparency)	Settings.Transparency = TRUE;
			else Settings.Transparency = FALSE;

			sal_AudioSetVolume(mMenuOptions.volume,mMenuOptions.volume);
			sal_CpuSpeedSet(mMenuOptions.cpuSpeed);	
			mFramesCleared = 0;
			if(mMenuOptions.soundEnabled) 	
				RunSound();
			else	RunNoSound();

			event=EVENT_NONE;
		}

		if(event==EVENT_EXIT_APP) break;	
	}

	MenuMessageBox("Saving SRAM...","","",MENU_MESSAGE_BOX_MODE_MSG);
	PSNESForceSaveSRAM();
	
	S9xGraphicsDeinit();
	S9xDeinitAPU();
	Memory.Deinit();

	sal_Reset();
  	return 0;
}
