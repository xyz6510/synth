
void wait_mutex(pthread_cond_t *cn,pthread_mutex_t *mx)
{
	pthread_mutex_lock(mx);
	pthread_cond_wait(cn,mx);
	pthread_mutex_unlock(mx);
}

void signal_mutex(pthread_cond_t *cn,pthread_mutex_t *mx)
{
	pthread_mutex_lock(mx);
	pthread_cond_signal(cn);
	pthread_mutex_unlock(mx);
}

void broadcast_mutex(pthread_cond_t *cn,pthread_mutex_t *mx)
{
	pthread_mutex_lock(mx);
	pthread_cond_broadcast(cn);
	pthread_mutex_unlock(mx);
}
