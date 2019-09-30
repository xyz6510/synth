
void wave(float *buf,int len)
{

	int wf=wave_form;
	float pw=pulse_width;
	float freq=frequency;
	float vol=volume;
	float vol_m=volume_master;

	static float po=0;
	static float fr=1;
	float ft=(float)snd_rate/fr;
	if (fr!=freq) {
		fr=po/ft;
		ft=(float)snd_rate/freq;
		po=ft*fr;
		fr=freq;
	}
	float fl=roundf(ft);
	float pa=ft/fl;

	float v=0;
	int i;
	for(i=0;i<len;i++) {
		switch (wf) {
			case w_sine: {
				float f=(po*2.0*M_PI)/ft;
				v=sinf(f);
				break;
			}
			case w_triangle: {
				float f=po/ft;
				v=1.0f-4.0f*fabsf(roundf(f-0.25f)-(f-0.25f));
				break;
			}
			case w_saw: {
				float f=po/ft;
				v=2.0f*(f-floorf(f+0.5f));
				break;
			}
			case w_square: {
				if (po<ft*pw) {
					v=1;
				} else v=-1;
				break;
			}
		}
		buf[i]=v;
		po+=pa;
		if (po>ft) po=(po-ft);
	}

	static float vol_old=0;
	if (vol_old!=vol) {
		float vol_change;
		float vol_start;
		int change_len;
		if (latency>2000) {
			change_len=len/(latency/2000);
		} else change_len=len;
		vol_start=vol_old;
		if (vol_old<vol) vol_change=(vol-vol_old)/change_len;
		else vol_change=-(vol_old-vol)/change_len;
		for (i=0;i<change_len;i++) {
			buf[i]=buf[i]*vol_start*vol_m;
			vol_start=vol_start+vol_change;
		}
		for (i=change_len;i<len;i++) {
			buf[i]=buf[i]*vol*vol_m;
		}
		vol_old=vol;
	} else {
		for (i=0;i<len;i++) {
			buf[i]=buf[i]*vol*vol_m;
		}
	}
}

void process()
{
	while(end==0) {
		wait_mutex(&cond1,&mutex1);
		snd_buf_idx^=1;
		float *buf=snd_buf[snd_buf_idx];
		wave(buf,snd_buf_len);
	}
}
