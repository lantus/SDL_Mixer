/*
    PLAYMUS:  A test application for the SDL mixer library.
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: playmus.c 4211 2008-12-08 00:27:32Z slouken $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#ifdef unix
#include <unistd.h>
#endif

#include "SDL.h"
#include "SDL_mixer.h"

#include <xetypes.h>
#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <usb/usbmain.h>
#include <console/console.h>
#include <xenon_smc/xenon_smc.h>
#include <xenon_soc/xenon_power.h>


static int audio_open = 0;
static Mix_Music *music = NULL;
static int next_track = 0;

void CleanUp(int exitcode)
{
	if( Mix_PlayingMusic() ) {
		Mix_FadeOutMusic(1500);
		SDL_Delay(1500);
	}
	if ( music ) {
		Mix_FreeMusic(music);
		music = NULL;
	}
	if ( audio_open ) {
		Mix_CloseAudio();
		audio_open = 0;
	}
	SDL_Quit();
	exit(exitcode);
}

void Usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [-i] [-l] [-8] [-r rate] [-c channels] [-b buffers] [-v N] [-rwops] <musicfile>\n", argv0);
}

void Menu(void)
{
	char buf[10];

	printf("Available commands: (p)ause (r)esume (h)alt volume(v#) > ");
	fflush(stdin);
	scanf("%s",buf);
	switch(buf[0]){
	case 'p': case 'P':
		Mix_PauseMusic();
		break;
	case 'r': case 'R':
		Mix_ResumeMusic();
		break;
	case 'h': case 'H':
		Mix_HaltMusic();
		break;
	case 'v': case 'V':
		Mix_VolumeMusic(atoi(buf+1));
		break;
	}
	printf("Music playing: %s Paused: %s\n", Mix_PlayingMusic() ? "yes" : "no", 
		   Mix_PausedMusic() ? "yes" : "no");
}

void IntHandler(int sig)
{
	switch (sig) {
	        case SIGINT:
			next_track++;
			break;
	}
}

int main(int argc, char *argv[])
{
	SDL_RWops *rwfp;
	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	int audio_buffers;
	int audio_volume = MIX_MAX_VOLUME;
	int looping = 0;
	int interactive = 0;
	int rwops = 0;
	int i;

     	xenon_make_it_faster(XENON_SPEED_FULL);
    	xenos_init(VIDEO_MODE_AUTO);
        xenon_sound_init();
    	console_init();
    	usb_init();
    	usb_do_poll(); 

	/* Initialize variables */
	audio_rate = 22050;
	audio_format = AUDIO_S16;
	audio_channels = 2;
	audio_buffers = 4096;


	/* Initialize the SDL library */
	if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return(255);
	}

	signal(SIGINT, IntHandler);
	signal(SIGTERM, CleanUp);

	/* Open the audio device */
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return(2);
	} else {
		Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
		printf("Opened audio at %d Hz %d bit %s (%s), %d bytes audio buffer\n", audio_rate,
			(audio_format&0xFF),
			(audio_channels > 2) ? "surround" : (audio_channels > 1) ? "stereo" : "mono", 
			(audio_format&0x1000) ? "BE" : "LE",
			audio_buffers );
	}
	audio_open = 1;

	/* Set the music volume */
	Mix_VolumeMusic(audio_volume);

	/* Set the external music player, if any */
	Mix_SetMusicCMD(getenv("MUSIC_CMD"));

	while (1) {
		next_track = 0;
		
		/* Load the requested music file */
		if ( rwops ) {
			rwfp = SDL_RWFromFile(argv[i], "rb");
			music = Mix_LoadMUS_RW(rwfp);
		} else {
			music = Mix_LoadMUS("uda:/chip4mat.mod");
		}
		if ( music == NULL ) {
			fprintf(stderr, "Couldn't load %s: %s\n",
				argv[i], SDL_GetError());
			CleanUp(2);
		}
		
		/* Play and then exit */
		printf("Playing...\n");
		Mix_FadeInMusic(music,looping,2000);
		while ( !next_track && (Mix_PlayingMusic() || Mix_PausedMusic()) ) {
			if(interactive)
				Menu();
			else
				SDL_Delay(100);
		}
		Mix_FreeMusic(music);
		if ( rwops ) {
			SDL_FreeRW(rwfp);
		}
		music = NULL;

		/* If the user presses Ctrl-C more than once, exit. */
		SDL_Delay(500);
		if ( next_track > 1 ) break;
		
		i++;
	}
	CleanUp(0);

	/* Not reached, but fixes compiler warnings */
	return 0;
}
