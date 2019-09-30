
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include <locale.h>
#include <ncurses.h>
#include <alsa/asoundlib.h>

#include "types.h"

int end=0;
unsigned int snd_rate=44100;
int chan=1;
unsigned int latency=21333;
unsigned int buffer_time;
int frames;

float *snd_buf[2];
int snd_buf_idx=0;

int snd_buf_len=0;
int buf_len=16384;

int stack_size=64*1024;

pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

float frequency=1;
float volume=0;
float volume_master=0.5;
float pulse_width=0.5;

wform wave_form=w_sine;

int kbd_repeat_wait=400;//in ms //ubuntu 400

#include "mutex.c"
#include "process.c"
#include "snd.c"

int main()
{
	setlocale(LC_CTYPE, "");

	snd_buf[0]=malloc(buf_len*sizeof(float));
	snd_buf[1]=malloc(buf_len*sizeof(float));
	memset(snd_buf[0],0,buf_len*sizeof(float));
	memset(snd_buf[1],0,buf_len*sizeof(float));

	pthread_mutex_init(&mutex1,NULL);
	pthread_mutex_init(&mutex2,NULL);
	pthread_cond_init(&cond1,NULL);
	pthread_cond_init(&cond2,NULL);

	pthread_t process_thread;
	pthread_attr_t process_attr;
	pthread_attr_init(&process_attr);
	pthread_attr_setstacksize(&process_attr,stack_size);
	pthread_create(&process_thread,&process_attr,(void*)&process,NULL);

	pthread_t snd_thread;
	pthread_attr_t snd_attr;
	pthread_attr_init(&snd_attr);
	pthread_attr_setstacksize(&snd_attr,stack_size);
	pthread_create(&snd_thread,&snd_attr,(void*)&snd,NULL);

	wait_mutex(&cond2,&mutex2);

	initscr();
	cbreak();
	noecho();
	nodelay(stdscr,TRUE);
	curs_set(0);
	clear();

	WINDOW *main_win=newwin(LINES,COLS,0,0);
	WINDOW *win=newwin(LINES-2,COLS-2,1,1);
	box(main_win,0,0);

	wprintw(win,"Press ESC to exit\n");
	wprintw(win,"rate:%i Hz,",snd_rate);
	wprintw(win," buffer_time:%0.3f ms,",buffer_time/1000.0);
	wprintw(win," latency:%0.3f ms,",latency/1000.0);
	wprintw(win," frames:%i",frames);

	char keys[3][9]={
					{"qwertyui"},
					{"asdfghjk"},
					{"zxcvbnm,"},
					};
	int fnum=sizeof(keys[0])-1;
	float freq[3][fnum];
	float fstep[8]={2,2.0393,1.9315,1.66,1.755,1.7687,1.8138,1.71428};
	char tones[8][3]={"Do","Re","Mi","Fa","So","La","Si","Do"};

	int i,j;
	float a=powf(2.0,1.0/12.0);
	for (i=0;i<3;i++) {
		for (j=0;j<strlen(keys[i]);j++) {
			float fr=261.626*powf(a,j*fstep[j]);
			if (i==0) fr=fr*2.0;
			if (i==2) fr=fr/2.0;
			freq[i][j]=fr;
		}
	}

	int y0=4;

	mvwaddstr(win,y0,2,"sine");	mvwaddstr(win,y0+1,3,"1");
	mvwaddstr(win,y0,8,"triangle");	mvwaddstr(win,y0+1,11,"2");
	mvwaddstr(win,y0,18,"saw");	mvwaddstr(win,y0+1,19,"3");
	mvwaddstr(win,y0,23,"square");	mvwaddstr(win,y0+1,25,"4");
	mvwaddstr(win,y0,40,"pwidth:");	mvwaddstr(win,y0+1,42,"[ ]");
	mvwaddstr(win,y0,55,"vol:");	mvwaddstr(win,y0+1,57,"- =");

	mvwaddstr(win,y0,1,"*");
	mvwprintw(win,y0,47,"%.0f",pulse_width*100);
	mvwprintw(win,y0,59,"%.0f",volume_master*100);

	int y=y0+3;
	int x=1;
	int pady=1;
	for (i=0;i<3;i++) {
		for (j=0;j<fnum;j++) {
			char buf[16];
			sprintf(buf,"%s",tones[j]);
			if (i==0) sprintf(buf,"%s+",tones[j]);
			if (i==2) sprintf(buf,"%s-",tones[j]);
			mvwaddnstr(win,y+i*(3+pady),x+1+j*8,buf,3);
			sprintf(buf,"%0.3f",freq[i][j]);
			mvwaddnstr(win,y+1+i*(3+pady),x+j*8,buf,7);
			mvwaddnstr(win,y+2+i*(3+pady),x+1+j*8,&keys[i][j],1);
		}
	}

	wrefresh(main_win);
	nodelay(main_win,TRUE);
	while(end==0) {
		static int cnt=0;
		static int count=0;
		wrefresh(win);
		wint_t key=0;
		wint_t kc;
		while(wget_wch(main_win,&kc)!=ERR) key=(key<<8)|kc;
		if (key!=0) {
			char buf[30];
			memset(buf,0x20,29);buf[29]=0;
			mvwaddnstr(win,2,0,buf,29);
			sprintf(buf,"key:%#x",key);
			mvwaddnstr(win,2,0,buf,15);
			sprintf(buf,"ckey:%lc",key);
			mvwaddnstr(win,2,15,buf,5+4);
		}
		if (key==0x1b) {//ESC
			end=1;
			break;
		}
		if ((key>='1')&&(key<='4')) {
			mvwaddstr(win,y0,1," ");
			mvwaddstr(win,y0,7," ");
			mvwaddstr(win,y0,17," ");
			mvwaddstr(win,y0,22," ");
			if (key=='1') {wave_form=w_sine;mvwaddstr(win,y0,1,"*");}
			if (key=='2') {wave_form=w_triangle;mvwaddstr(win,y0,7,"*");}
			if (key=='3') {wave_form=w_saw;mvwaddstr(win,y0,17,"*");}
			if (key=='4') {wave_form=w_square;mvwaddstr(win,y0,22,"*");}
		}
		if (key=='-') {
			volume_master=volume_master-0.01;
			if (volume_master<0) volume_master=0;
			mvwprintw(win,y0,59,"%.0f ",volume_master*100);
		}
		if (key=='=') {
			volume_master=volume_master+0.01;
			if (volume_master>1) volume_master=1;
			mvwprintw(win,y0,59,"%.0f ",volume_master*100);
		}
		if (key=='[') {
			pulse_width=pulse_width-0.01;
			if (pulse_width<0.01) pulse_width=0.01;
			mvwprintw(win,y0,47,"%.0f ",pulse_width*100);
		}
		if (key==']') {
			pulse_width=pulse_width+0.01;
			if (pulse_width>0.99) pulse_width=0.99;
			mvwprintw(win,y0,47,"%.0f ",pulse_width*100);
		}
		static int oyp=-1,oxp=-1;
		if (key!=0) {
			for (i=0;i<3;i++) {
				for (j=0;j<fnum;j++) {
					if (key==keys[i][j]) {
						int xp=x+0+j*8;
						int yp=y+i*(3+pady);
						if (oyp!=-1) mvwaddnstr(win,oyp,oxp," ",1);
						oyp=yp;oxp=xp;
						mvwaddnstr(win,yp,xp,"*",1);
						frequency=freq[i][j];
						volume=1;
						cnt=0;
						count=1;
						goto exit_loop;
					}
				}
			}
		} else {
			if (count) {
				cnt++;
				if (cnt>kbd_repeat_wait) {
					cnt=0;
					count=0;
					volume=0;
					mvwaddnstr(win,oyp,oxp," ",1);
				}
			}
		}
		exit_loop:
		usleep(1000);
	}
	endwin();

	pthread_join(snd_thread,NULL);
	signal_mutex(&cond1,&mutex1);
	pthread_join(process_thread,NULL);

	return 42;
}
