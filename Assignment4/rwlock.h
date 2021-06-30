#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
struct read_write_lock
{
	sem_t readLock;
	sem_t writeLock;
	sem_t writePred;
	sem_t writeCount;
	int nReaders;
	int nWriters;
};

void InitalizeReadWriteLock(struct read_write_lock * rw);
void ReaderLock(struct read_write_lock * rw);
void ReaderUnlock(struct read_write_lock * rw);
void WriterLock(struct read_write_lock * rw);
void WriterUnlock(struct read_write_lock * rw);
