
void snd()
{
	snd_pcm_t *pch;
	snd_pcm_uframes_t frames_s;
	snd_pcm_hw_params_t *snd_params=NULL;
	unsigned int periods=3;

	snd_pcm_open(&pch,"default",SND_PCM_STREAM_PLAYBACK,0);
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pch,snd_params);
	snd_pcm_hw_params_set_access(pch,snd_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pch,snd_params,SND_PCM_FORMAT_FLOAT_LE);
	snd_pcm_hw_params_set_channels(pch,snd_params,chan);
	snd_pcm_hw_params_set_rate_near(pch,snd_params,&snd_rate,0);
	snd_pcm_hw_params_set_periods_near(pch,snd_params,&periods,0);
	snd_pcm_hw_params_set_period_time_near(pch,snd_params,&latency,0);
	snd_pcm_hw_params(pch,snd_params);

	snd_pcm_hw_params_get_buffer_time(snd_params,&buffer_time,0);
	snd_pcm_hw_params_get_period_size(snd_params,&frames_s,0);
	snd_pcm_hw_params_get_period_time(snd_params,&latency,0);

	frames=frames_s;
	snd_buf_len=frames_s*chan;

	signal_mutex(&cond2,&mutex2);
	while(end==0) {
		float *buf=snd_buf[snd_buf_idx];
		signal_mutex(&cond1,&mutex1);
		int res;
		if ((res=snd_pcm_writei(pch,buf,frames_s))==-EPIPE) {
			printf(":xrun\n");
			snd_pcm_prepare(pch);
		} else if (res<0) {
			printf(":error write PCM device. %s\n",snd_strerror(res));
		}
	}
	snd_pcm_close(pch);
}
