/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include "driver.h"
#include <allegro.h>
#include <dos.h>
#include <signal.h>
#include <time.h>


int  msdos_init_seal (void);
int  msdos_init_sound(void);
void msdos_init_input(void);
void msdos_shutdown_sound(void);
void msdos_shutdown_input(void);
int  frontend_help (int argc, char **argv);
void parse_cmdline (int argc, char **argv, struct GameOptions *options, int game);

/* platform independent options go here */
struct GameOptions options;

int  ignorecfg;

/* avoid wild card expansion on the command line (DJGPP feature) */
char **__crt0_glob_function(void)
{
	return 0;
}

static void signal_handler(int num)
{
   allegro_exit();

   signal(num, SIG_DFL);
   raise(num);
}

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	if (msdos_init_sound())
		return 1;
	msdos_init_input();
	return 0;
}


/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	msdos_shutdown_sound();
	msdos_shutdown_input();
}

/* fuzzy string compare, compare short string against long string */
/* e.g. astdel == "Asteroids Deluxe"            return 0 on match */
int fuzzycmp (const char *s, const char *l)
{
	for (; *s && *l; l++)
	{
		if (*s == *l)
			s++;
		else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
			s++;
		else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
			s++;
	}

	if (*s)
		return 1;
	else
		return 0;
}



int main (int argc, char **argv)
{
	int res, i, j, game_index;

	/* these two are not available in mame.cfg */
	ignorecfg = 0;
	errorlog = options.errorlog = 0;

	for (i = 1;i < argc;i++) /* V.V_121997 */
	{
		if (stricmp(argv[i],"-ignorecfg") == 0) ignorecfg = 1;
		if (stricmp(argv[i],"-log") == 0)
			errorlog = options.errorlog = fopen("error.log","wa");
	}

	allegro_init();

	/* Allegro changed the signal handlers... change them again to ours, to */
	/* avoid the "Shutting down Allegro" message which confuses users into */
	/* thinking crashes are caused by Allegro. */
	signal(SIGABRT, signal_handler);
	signal(SIGFPE,  signal_handler);
	signal(SIGILL,  signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGQUIT, signal_handler);


	set_config_file ("mame.cfg");

	/* Initialize the audio library */
	if (msdos_init_seal())
	{
		printf ("Unable to initialize SEAL\n");
		return (1);
	}

	/* check for frontend options */
	res = frontend_help (argc, argv);

	/* if frontend options were used, return to DOS with the error code */
	if (res != 1234)
		exit (res);

	/* take the first commandline argument without "-" as the game name */
	for (j = 1; j < argc; j++)
		if (argv[j][0] != '-') break;

	game_index = -1;

	/* do we have a driver for this? */
#ifdef MAME_DEBUG
	/* pick a random game */
	if (stricmp(argv[j],"random") == 0)
	{
		struct timeval t;

		i = 0;
		while (drivers[i]) i++;	/* count available drivers */

		gettimeofday(&t,0);
		srand(t.tv_sec);
		game_index = rand() % i;

		printf("Running %s (%s) [press return]\n",drivers[game_index]->name,drivers[game_index]->description);
		getchar();
	}
	else
#endif
	{
		for (i = 0; drivers[i] && (game_index == -1); i++)
		{
			if (stricmp(argv[j],drivers[i]->name) == 0)
			{
				game_index = i;
				break;
			}
		}

		if (game_index == -1)
		{
			/* educated guess on what the user wants to play */
			for (i = 0; drivers[i] && (game_index == -1); i++)
			{
				if (fuzzycmp(argv[j], drivers[i]->description) == 0)
				{
					game_index = i;
					break;
				}
			}
		}
	}

	if (game_index == -1)
	{
		printf("Game \"%s\" not supported\n", argv[j]);
		return 1;
	}

	/* parse generic (os-independent) options */
	parse_cmdline (argc, argv, &options, game_index);

{	/* Mish:  I need sample rate initialised _before_ rom loading for optional rom regions */
	extern int soundcard;

	if (soundcard == 0) {    /* silence, this would be -1 if unknown in which case all roms are loaded */
		Machine->sample_rate = 0; /* update the Machine structure to show that sound is disabled */
		options.samplerate=0;
	}
}

	/* handle record and playback. These are not available in mame.cfg */
	for (i = 1; i < argc; i++)
	{
		if (stricmp(argv[i],"-record") == 0)
		{
			i++;
			if (i < argc)
				options.record = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,1);
		}
		if (stricmp(argv[i],"-playback") == 0)
		{
			i++;
			if (i < argc)
				options.playback = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,0);
		}
	}

	/* go for it */
	res = run_game (game_index , &options);

	/* close open files */
	if (options.errorlog) fclose (options.errorlog);
	if (options.playback) osd_fclose (options.playback);
	if (options.record)   osd_fclose (options.record);

	exit (res);
}
