/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2007 - 2008, Russell Bryant
 *
 * Russell Bryant <russell@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*!
 * \file
 * \brief Premier Application
 *
 * \author Debreuil Philippe <phd@debreuil.fr>
 *
 * This is an application to play foreground audio over background.
 *
 * \extref http://www.debreuil.fr/
 *
 * \note To install Premier, check it out of the following repository:
 * <code>$ git clone https://git.debreuil.fr/premier</code>
 *
 * \ingroup applications
 */

/*** MODULEINFO
 ***/

#include "asterisk.h"

#include <limits.h>
#include "asterisk/paths.h"	/* use ast_config_AST_DATA_DIR */
#include "asterisk/module.h"
#include "asterisk/channel.h"
#include "asterisk/strings.h"
#include "asterisk/lock.h"
#include "asterisk/app.h"
#include "asterisk/pbx.h"
#include "asterisk/audiohook.h"
#include "asterisk/format_cache.h"

#include "asterisk/stasis_channels.h"
#include "asterisk/cli.h"
#include "asterisk/causes.h"

/*** DOCUMENTATION
	<application name="PREMIER" language="en_US">
		<synopsis>
			PREMIER Audio studio
		</synopsis>
		<syntax>
			<parameter name="filenames" required="true">
				<argument name="filename" required="true" />				
			</parameter>
		</syntax>
		<description>
			<para>When executing this application blabla...</para>
		</description>
	</application>

	<manager name="PremierStatus" language="en_US">
		<synopsis>
			Show Premier status.
		</synopsis>
		<syntax>
			<xi:include xpointer="xpointer(/docs/manager[@name='Login']/syntax/parameter[@name='ActionID'])" />
		</syntax>
		<description>
			<para>Check the status of premier.</para>
		</description>
	</manager>	

	<manager name="PremierPlayForeground" language="en_US">
		<synopsis>
			Show Premier status.
		</synopsis>
		<syntax>
			<xi:include xpointer="xpointer(/docs/manager[@name='Login']/syntax/parameter[@name='ActionID'])" />

			<parameter name="channel" required="true">
				<para>The channel.</para>
			</parameter>
			<parameter name="filename" required="true">
				<para>The filename to be played.</para>
			</parameter>

		</syntax>
		<description>
			<para>Check the status of premier.</para>
		</description>
	</manager>	

	<managerEvent language="en_US" name="PremierForegroundEnded">
		<managerEventInstance class="EVENT_FLAG_CALL">
			<synopsis>Raised when foreground has been played.</synopsis>
		</managerEventInstance>
	</managerEvent>	
	<managerEvent language="en_US" name="PremierForegroundStarted">
		<managerEventInstance class="EVENT_FLAG_CALL">
			<synopsis>Raised when foreground start playing.</synopsis>
		</managerEventInstance>
	</managerEvent>	
 ***/

//#define MUSIC_FILE "/home/phd/Music/MOH/portorico.wav"
//#define MUSIC_FILE "/home/phd/Music/WAV/laissezvotreMessage.wav"
//#define MUSIC_FILE "/home/phd/Music/WAV/STI_predec_CNIL.wav"
//#define SOUNDS_DIR "/var/lib/asterisk/sounds/"
//#define BACKGROUND_MUSIC_FILE SOUNDS_DIR "attente-flutedepan.wav"
//#define BACKGROUND_MUSIC_FILE SOUNDS_DIR "earth.wav"
//#define FOREGROUND_MUSIC_FILE SOUNDS_DIR"tarbe.wav"
//#define FOREGROUND_MUSIC_FILE SOUNDS_DIR "lignes-occupees.wav"
//#define FOREGROUND_MUSIC_FILE SOUNDS_DIR "horaires.wav"
//#define FOREGROUND_MUSIC_FILE "peugeot.wav"
#define MAX_DELTA_VOL 3		// decrease volume 1 by 1 till
#define MSEC_DELTA_VOL 2000 // millisec between decrease by 1
#define CONCISE_FORMAT_STRING "%s!%s!%s!%d!%s!%s!%s!%s!%s!%s!%d!%s!%s!%s\n"

#pragma GCC diagnostic ignored "-Wunused-result"

static const char premier_app[] = "PREMIER";

float square_sum(short int *data, int nb_short);

static struct ast_manager_event_blob *premier_foreground_ended_to_ami(struct stasis_message *message)
{
	struct ast_manager_event_blob *res;
	struct ast_tm tm;
	char timebuf[30];
	struct timeval now = ast_tvnow();
	struct ast_channel_blob *obj = stasis_message_data(message);

	RAII_VAR(struct ast_str *, channel_string, NULL, ast_free);
	channel_string = ast_manager_build_channel_state_string(obj->snapshot);

	ast_localtime(&now, &tm, NULL);
	ast_strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);		
	ast_verb(3, "PremierForegroundEnded [%s]\n", timebuf);
	res = ast_manager_event_blob_create ( EVENT_FLAG_CALL, "PremierForegroundEnded"
		,"date: %s\r\n" , timebuf
		,"channel: %s\r\n" , channel_string
		);
	return res;
}

static struct ast_manager_event_blob *premier_foreground_started_to_ami(struct stasis_message *message)
{
	/*struct ast_manager_event_blob *res;
	struct ast_tm tm;
	struct timeval now = ast_tvnow();
	char timebuf[30];
	ast_localtime(&now, &tm, NULL);
	ast_strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm);		
	ast_verb(3, "PremierForegroundStarted [%s]\n", timebuf);
	res = ast_manager_event_blob_create ( EVENT_FLAG_CALL, "PremierForegroundStarted",
		"date: %s\r\n" , timebuf
		);		
	return res;*/
	struct ast_manager_event_blob *res;
	struct ast_tm tm;
	char timebuf[30];
	struct timeval now = ast_tvnow();
	struct ast_channel_blob *obj = stasis_message_data(message);

	RAII_VAR(struct ast_str *, channel_string, NULL, ast_free);
	channel_string = ast_manager_build_channel_state_string(obj->snapshot);

	ast_localtime(&now, &tm, NULL);
	ast_strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);		
	ast_verb(3, "PremierForegroundEnded [%s]\n", timebuf);
	res = ast_manager_event_blob_create ( EVENT_FLAG_CALL, "PremierForegroundStarted"
		,"date: %s\r\n" , timebuf
		,"channel: %s\r\n" , channel_string
		);
	return res;	
}

STASIS_MESSAGE_TYPE_DEFN_LOCAL(premier_foreground_started_type,
	.to_ami = premier_foreground_started_to_ami,
);

STASIS_MESSAGE_TYPE_DEFN_LOCAL(premier_foreground_ended_type,
	.to_ami = premier_foreground_ended_to_ami, 
);

static int timed_read(int fd, void *data, int datalen, int timeout /*, int pid */)
{
	int res;
	int i;
	struct pollfd fds[1];
	if (fd == -1)
	{
		perror("open");
		return 0;
	}
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	for (i = 0; i < timeout; i++)
	{
		res = ast_poll(fds, 1, 1000);
		if (res > 0)
		{
			break;
		}
		else if (res == 0)
		{
			if (errno == ESRCH)
			{
				return -1;
			}
		}
		else
		{
			ast_log(LOG_NOTICE, "error polling mpg123: %s\n", strerror(errno));
			return -1;
		}
	}

	if (i == timeout)
	{
		ast_log(LOG_NOTICE, "Poll timed out.\n");
		return -1;
	}

	return read(fd, data, datalen);
}

float square_sum(short int *data, int nb_short)
{
	float sum = 0.0;
	for (int k = 0; k < nb_short; k++)
	{
		float val = data[k] / 32768.0;
		sum += val * val;
	}
	return sum;
}

static int premier_exec(struct ast_channel *chan, const char *data)
{
	int res = 0;
	//char dir_path[PATH_MAX];
	char bg_filepath[PATH_MAX];
	char fg_filepath[PATH_MAX];								
	char* tmp=NULL;
	char* front=NULL;
	char* back=NULL;
	int fwav_background=-1;
	unsigned char bg_wav_header[56];

	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(filenames);
		AST_APP_ARG(options);
	);
	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "Premier requires an argument (filename)\n");
		return -1;
	}		
	tmp = ast_strdupa(data);
	AST_STANDARD_APP_ARGS(args, tmp);

	if (ast_channel_state(chan) != AST_STATE_UP) {
		res = ast_answer(chan);
		ast_log(LOG_WARNING, "Premier answers channel %d\n", res);
	}

	if (!res) {
		back = args.filenames;
		front = strsep(&back, "&");

		if ( front && strlen(front) ){

			ast_verb(3, "Premier first filename argument is [%s]\n", front);
			snprintf(bg_filepath, sizeof(bg_filepath), "%s/sounds/%s.wav", ast_config_AST_DATA_DIR, front);
			ast_verb(3, "Premier bg_filepath is [%s]\n", bg_filepath);

			fwav_background = open(bg_filepath, O_RDONLY | O_NONBLOCK);			
			if ( fwav_background>0 ) {
				ast_verb(3, "fwav_background opened (fd=%d) [%s]\n", fwav_background, bg_filepath);
				read(fwav_background, bg_wav_header, sizeof(bg_wav_header));
			}else{
				ast_log(LOG_WARNING, "Premier could not open background sound file [%s] \n",bg_filepath);				
				return -1;				
			}
		}else{
			ast_log(LOG_WARNING, "Premier invalid argument\n");
			return -1;	
		}
	}else{
		ast_log(LOG_WARNING, "Premier could not answer channel\n");
		return -1;	
	}

	if (fwav_background > 0)
	{
		int stop=0;
		struct timeval now_in;
		struct timeval prec_in;
		int total_ms = 0;
		int ramped_start_ms = 0;
		int ramped_end_ms = 0;
		int fwav_foreground = -1;
		int sampling_rate = 8000;
		int timeout = 2;
		int ms = -1;

		struct timeval next;
		struct ast_frame *f;
		struct myframe
		{
			struct ast_frame f;
			char offset[AST_FRIENDLY_OFFSET];
			short frdata[160];
		} myf = {
			.f = {
				0,
			},
		};		

		myf.f.frametype = AST_FRAME_VOICE;
		myf.f.subclass.format = ast_format_slin; //ast_format_alaw;//write_format;
		myf.f.mallocd = 0;
		myf.f.offset = 0; //AST_FRIENDLY_OFFSET;
		myf.f.src = __PRETTY_FUNCTION__;
		myf.f.delivery.tv_sec = 0;
		myf.f.delivery.tv_usec = 0;
		myf.f.data.ptr = myf.frdata;

		/* Wait 1000 ms first */
		next = ast_tvnow();
		next.tv_sec += 1;
		gettimeofday(&now_in, NULL);
		gettimeofday(&prec_in, NULL);

		while (!stop) 
		{
			ms = ast_tvdiff_ms(next, ast_tvnow()); //ast_verb(3, "ms=%d ms total_ms=%d ramped_ms=%d\n",ms , total_ms, ramped_ms);
			if (ms <= 0)
			{
				int nb_samples = 0;
				res = timed_read(fwav_background /*fds[0] */, myf.frdata, sizeof(myf.frdata), timeout /*, pid */);
				nb_samples = res / 2;
				if (res <= 0){
					close(fwav_background);
					fwav_background = open(bg_filepath, O_RDONLY | O_NONBLOCK);
					if (fwav_background>0){
						unsigned char fg_wav_header[56];
						ast_verb(3, "OK fwav_background REopened (fd=%d) [%s]\n", fwav_background, bg_filepath);
						read(fwav_background, fg_wav_header, sizeof(fg_wav_header));
						res = timed_read(fwav_background /*fds[0] */, myf.frdata, sizeof(myf.frdata), timeout /*, pid */);
						nb_samples = res / 2;
					}else{						
						ast_log(LOG_WARNING, "NOK Premier could not reopen background sound file [%s] \n",bg_filepath);
						if (fwav_foreground > 0) close(fwav_foreground);
						return -1;						
					}
				}
				
				if (res > 0)
				{
					double tdif = 0.0;
					gettimeofday(&now_in, NULL);
					tdif = (double)now_in.tv_sec - (double)prec_in.tv_sec;
					tdif = 1000.0 * tdif;
					tdif += ((double)now_in.tv_usec - (double)prec_in.tv_usec) / 1000;
					memcpy(&prec_in, &now_in, sizeof(struct timeval)); // now devient prec
					//ast_verb(3,"elapsed=%.0lf ms\n" , tdif);  // 20 ms
					total_ms += (int)tdif;

					//ast_verb(3, "read %d bytes in fd %d ( %d samples slin short)\n", res,fwav_background,nb_samples);
					/*    -- read 320 bytes in fd 33 ( 160 samples slin short)
    -- read 320 bytes in fd 33 ( 160 samples slin short)
    -- read 320 bytes in fd 33 ( 160 samples slin short)
    -- read 320 bytes in fd 33 ( 160 samples slin short)*/

					/*for (int k=0; k<res; k++){ myf.frdata[k]= (int) ( gain * (float)myf.frdata[k] );					}*/

					if (1)
					{
						if (fwav_foreground <= 0)
						{
							const char *str_var_foreground = pbx_builtin_getvar_helper(chan, "FOREGROUND_MUSIC_FILE");//ast_channel_unlock(chan);
							if (str_var_foreground != NULL && strlen(str_var_foreground))
							{
								snprintf(fg_filepath, sizeof(fg_filepath), "%s/sounds/%s", ast_config_AST_DATA_DIR, str_var_foreground);
								ast_verb(3, "Premier fg_filepath is [%s]\n", fg_filepath);

								fwav_foreground = open(fg_filepath, O_RDONLY | O_NONBLOCK);								
								if (fwav_foreground <= 0)
								{
									ast_verb(3, "ERROR opening fg_filepath [%s]\n", fg_filepath);
									pbx_builtin_setvar_helper(chan, "FOREGROUND_MUSIC_FILE", "");
								}else{
									unsigned char entete_wav[56];
									read(fwav_foreground, entete_wav, sizeof(entete_wav));
									ramped_start_ms = total_ms;
									ast_verb(3, "OK open fg_filepath [%s]\n", fg_filepath);

									if(1){
										struct ast_tm tm;
										char timebuf[30];
										struct timeval now = ast_tvnow();
										ast_localtime(&now, &tm, NULL);
										ast_strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);	
										
	
	
										manager_event(EVENT_FLAG_CALL, "PremierForegroundStarted", "channel: %s\r\nfilename: %s\r\n", ast_channel_name(chan), fg_filepath);

									}
								}
/*    -- fwav_background opened (fd=33) [/var/lib/asterisk/sounds/earth.wav]
       > 0x7f3680006e10 -- Strict RTP switching to RTP target address 192.168.8.249:3000 as source
       > 0x7f3680006e10 -- Strict RTP learning complete - Locking on source address 192.168.8.249:3000
    -- var_foreground FOUND [/var/lib/asterisk/sounds/lignes-occupees.wav]
    -- fwav_foreground (fd 34) JUST opened [/var/lib/asterisk/sounds/lignes-occupees.wav]
    -- 320 read bytes in fd 34 ( 160 samples slin short) 
    -- 320 read bytes in fd 34 ( 160 samples slin short) 
    -- 320 read bytes in fd 34 ( 160 samples slin short)  */
							}
						}
						if (fwav_foreground > 0)
						{
							int depuis_ms = total_ms - ramped_start_ms;
							int delta_vol = depuis_ms / MSEC_DELTA_VOL;

							if (delta_vol > MAX_DELTA_VOL)
								delta_vol = MAX_DELTA_VOL;
							else{
								//ast_verb(3, "START depuis_ms=%d delta_vol=%d\n", depuis_ms, delta_vol);
							}
							ast_frame_adjust_volume(&myf.f, -1 * delta_vol);

							if (delta_vol == MAX_DELTA_VOL) // decreasing backgrounf volume is done
							{
								short frdata[160]={0};
								int nb_read_fwav_foreground = read(fwav_foreground, frdata, sizeof(frdata));
								nb_samples = nb_read_fwav_foreground / 2;

								//ast_verb(3, "%d read bytes in fd %d ( %d samples slin short) \n", nb_read_fwav_foreground, fwav_foreground , nb_samples);
								if (nb_read_fwav_foreground <= 0)
								{
									//const char *str_var_foreground = pbx_builtin_getvar_helper(chan, "FOREGROUND_MUSIC_FILE");
									ramped_end_ms = total_ms;
									fwav_foreground = -1;
									pbx_builtin_setvar_helper(chan, "FOREGROUND_MUSIC_FILE", "");																				
									ast_verb(3, "Foreground played res=%d fwav_background=%d\n", res, fwav_background);																				
									manager_event(EVENT_FLAG_CALL, "PremierForegroundEnded","channel: %s\r\nfilename: %s\r\n",ast_channel_name(chan) ,fg_filepath);									
								}else{// MIXING !!!
									//float pow_background = square_sum(myf.frdata, nb_samples);
									float pow_foreground = square_sum(frdata, nb_samples);									
									if (pow_foreground > 0.000001){ 			
										for (int k = 0; k < nb_samples; k++){
											myf.frdata[k] = (myf.frdata[k] + frdata[k]) / 2;
										}
									}else{/* foreground very silent ! do not mix !*/							
										//ast_verb(3, "%.8f <> %.8f\n", pow_background, pow_foreground);
									}
								}
							}
						}
						else if (ramped_end_ms > 0)
						{
							int depuis_ms = total_ms - ramped_end_ms;
							int delta_vol = MAX_DELTA_VOL - (depuis_ms / MSEC_DELTA_VOL);
							if (delta_vol <= 0){ // increasing backgrounf volume is done
								delta_vol = 0;
								ramped_end_ms = 0;								
							}
							//ast_verb(3, "END depuis_ms=%d delta_vol=%d\n", depuis_ms, delta_vol);
							ast_frame_adjust_volume(&myf.f, -1 * delta_vol);
						}
					}
					myf.f.datalen = res;
					myf.f.samples = res / 2;
					if (ast_write(chan, &myf.f) < 0)
					{
						ast_debug(1, "ast_write failed\n");
						res = -1;
						break;
					}
				}else{
					res = 0;
				}
				next = ast_tvadd(next, ast_samp2tv(myf.f.samples, sampling_rate));
			}
			else
			{
				ms = ast_waitfor(chan, ms);
				if (ms < 0)
				{
					ast_debug(1, "Hangup detected\n");
					res = -1;
					stop = 1;
					break;
				}
				if (ms)
				{
					f = ast_read(chan);
					if (!f)
					{
						ast_verb(3, "Null frame == hangup() detecte\n");
						res = -1;
						stop = 1;									
						break;
					}else{
						if (f->frametype == AST_FRAME_DTMF)
						{
							if (f->subclass.integer == '#') 
								ast_verb(3 ,"User pressed #\n");
							else 
								ast_verb(3, "User pressed key [%d]\n", f->subclass.integer);
							ast_frfree(f);
							res = 0;
						}
						ast_frfree(f);
					}
				}
			}
		}
	}
	ast_verb(3, "premier() return res=%d\n",res);
	return res;
}

static char *complete_premier(const char *line, const char *word, int pos, int state)
{	/* 0 - premier; 1 - play; 2 - channel; 3 - filename;  */
	switch (pos)	{
	case 3: /* only one possible match, "to" */
		return ast_strdup(" SIP/");
	case 4: /* <queue> */
		return ast_strdup("toto.wav");	
	default:
		return NULL;
	}
}

static char *premier_show(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	//int delta_volume = 0;
	//ast_cli(a->fd, "cmd=%d CLI_INIT=%d CLI_GENERATE=%d\n", cmd, CLI_INIT, CLI_GENERATE);
	//ast_verb(3, "cmd=%d CLI_INIT=%d CLI_GENERATE=%d\n", cmd, CLI_INIT, CLI_GENERATE);
	switch (cmd)
	{
	case CLI_INIT:
		e->command = "premier play";
		e->usage = "Usage: premier play channel file.wav\n"
				   "ex: asterisk -rx 'premier play SIP/9003-00000004 peugeot.wav' \n"
				   "\n";

		return NULL;
	case CLI_GENERATE:
		return complete_premier(a->line, a->word, a->pos, a->n);
	}

	if (1)
  {
      if (NULL != a)
      {
          //char *mymodule = NULL;
          //char *mycmd = NULL;
          const char *mywav;
          ast_cli(a->fd, "a->argc is [%d]\n", a->argc);
          if (a->argc >= 1)
          {
              //mymodule = a->argv[0];
              ast_cli(a->fd, "module is [%s]\n", a->argv[0]); // premier
          }
          if (a->argc >= 2)
          {
              //mycmd = a->argv[1];
              ast_cli(a->fd, "command is [%s]\n", a->argv[1]); /// play
          }
          if (a->argc >= 4)
          {
              mywav = a->argv[3];
              ast_cli(a->fd, "sound file is [%s]\n", a->argv[3]); // fichier wav
              /*}
                if (a->argc >= 3)
                {*/
              const char *mychan;
              mychan = a->argv[2];
              ast_cli(a->fd, "channel is [%s]\n", a->argv[2]);
              /*if (sscanf(a->argv[2], "%d", &delta_volume) == 1){
                ast_cli(a->fd, "delta_volume is [%d]\n", delta_volume);
                }*/
              struct ast_channel *chan2 = NULL;
              if (!(chan2 = ast_channel_get_by_name(mychan)))
              {
                  ast_log(LOG_WARNING, "No such channel: %s\n", mychan);
              }
              else
              {
                  ast_verb(3, "OK found channel [%s]\n", mychan);
                  //pbx_builtin_setvar_helper(chan2, "BACKGROUND", PROMPT_FILE);
                  pbx_builtin_setvar_helper(chan2, "FOREGROUND_MUSIC_FILE", mywav); //FOREGROUND_MUSIC_FILE);
              }
          }
#if 0
          if(0){
              struct ao2_container *channels;
              struct ao2_iterator it_chans;
              struct stasis_message *msg;
              int concise = 0, verbose = 0, count = 0;

              if (!(channels = stasis_cache_dump(ast_channel_cache_by_name(), ast_channel_snapshot_type())))
              {
                  ast_cli(a->fd, "Failed to retrieve cached channels\n");
                  return CLI_SUCCESS;
              }
              it_chans = ao2_iterator_init(channels, 0);
              for (; (msg = ao2_iterator_next(&it_chans)); ao2_ref(msg, -1))
              {
                  struct ast_channel_snapshot *cs = stasis_message_data(msg);
                  char durbuf[16] = "-";
                  if (!ast_tvzero(cs->creationtime))
                  {
                      int duration = (int)(ast_tvdiff_ms(ast_tvnow(), cs->creationtime) / 1000);
                      int durh = duration / 3600;
                      int durm = (duration % 3600) / 60;
                      int durs = duration % 60;
                      snprintf(durbuf, sizeof(durbuf), "%02d:%02d:%02d", durh, durm, durs);
                      ast_cli(a->fd, CONCISE_FORMAT_STRING, cs->name, cs->context, cs->exten, cs->priority, ast_state2str(cs->state),
                              S_OR(cs->appl, "(None)"),
                              cs->data,
                              cs->caller_number,
                              cs->accountcode,
                              cs->peeraccount,
                              cs->amaflags,
                              durbuf,
                              cs->bridgeid,
                              cs->uniqueid);
                  }
              }
              ao2_iterator_destroy(&it_chans);
          }
#endif
          return CLI_SUCCESS;
      }
      else
      {
          return CLI_SHOWUSAGE;
      }
  }
	return CLI_FAILURE;
}

static struct ast_cli_entry cli_queue[] = {
	AST_CLI_DEFINE(premier_show, "Show status of a Premier module"),
};

/*! \brief Premier status info via AMI */

static int unload_module(void)
{
	int res = ast_unregister_application(premier_app);
	STASIS_MESSAGE_TYPE_CLEANUP(premier_foreground_ended_type);
	STASIS_MESSAGE_TYPE_CLEANUP(premier_foreground_started_type);
	//STASIS_MESSAGE_TYPE_CLEANUP(premier_foreground_played_type);
	//STASIS_MESSAGE_TYPE_CLEANUP(premier_foreground_played_type);
	return res;
}

static int premier_play_foreground(struct mansession *s, const struct message *m)
{
	const char *filename, *channel_name;
	//int paused, penalty = 0;

	filename = astman_get_header(m, "filename");
	channel_name = astman_get_header(m, "channel");
	if (ast_strlen_zero(channel_name)) {
		astman_send_error(s, m, "'channel_name' not specified.");
		return 0;
	}

	if (ast_strlen_zero(filename)) {
		astman_send_error(s, m, "'filename' not specified.");
		return 0;
	}

	if(1){

				struct ast_channel *chan2 = NULL;
				if (!(chan2 = ast_channel_get_by_name(channel_name)))
				{
					ast_log(LOG_WARNING, "No such channel: %s\n", channel_name);
				}
				else
				{
					ast_verb(3, "OK found channel [%s]\n", channel_name);
					//pbx_builtin_setvar_helper(chan2, "BACKGROUND", PROMPT_FILE);
					//pbx_builtin_setvar_helper(chan2, BACKGROUND_MUSIC_FILE, "azerty");
					pbx_builtin_setvar_helper(chan2, "FOREGROUND_MUSIC_FILE", filename); //FOREGROUND_MUSIC_FILE);
				}		

		

		astman_send_ack(s, m, "OK Premier Playing filename");
	}else{
		astman_send_error(s, m, "NOK Premier Out of memory");
	}
	return 0;
}

static int load_module(void)
{
	int err = 0;
	if (ast_register_application_xml(premier_app, premier_exec))
	{
		return AST_MODULE_LOAD_DECLINE;
	}
	err |= ast_cli_register_multiple(cli_queue, ARRAY_LEN(cli_queue));
	err |= STASIS_MESSAGE_TYPE_INIT(premier_foreground_ended_type);
	err |= STASIS_MESSAGE_TYPE_INIT(premier_foreground_started_type);
	err |= ast_manager_register_xml("PremierPlayForeground", EVENT_FLAG_CALL, premier_play_foreground);

	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD_EXTENDED(ASTERISK_GPL_KEY, "PREMIER Sound File Playback over Background Application");
