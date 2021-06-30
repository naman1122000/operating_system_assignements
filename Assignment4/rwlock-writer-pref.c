#include "rwlock.h"

void InitalizeReadWriteLock(struct read_write_lock * rw)
{
  //	Write the code for initializing your read-write lock.
	sem_init(&rw->readLock, 0, 1);
	sem_init(&rw->writeLock, 0, 1);
	sem_init(&rw->writePred, 0, 1);
	sem_init(&rw->writeCount, 0, 1);
	rw->nReaders = 0;
	rw->nWriters = 0;
}

void ReaderLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the reader.
	sem_wait(&rw->writePred);	
	sem_wait(&rw->readLock);
	rw->nReaders++;
	if(rw->nReaders == 1)
		sem_wait(&rw->writeLock);
	sem_post(&rw->readLock);
	sem_post(&rw->writePred);
}

void ReaderUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the reader.
	sem_wait(&rw->readLock);
	rw->nReaders--;
	if(rw->nReaders == 0){
		sem_post(&rw->writeLock);
	}
	sem_post(&rw->readLock);
}

void WriterLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the writer.
	sem_wait(&rw->writeCount);
	rw->nWriters++;
	if(rw->nWriters == 1) 
		sem_wait(&rw->writePred);
	sem_post(&rw->writeCount);
	
	sem_wait(&rw->writeLock);

	sem_wait(&rw->writeCount);
	rw->nWriters--;
	if(rw->nWriters == 0)
		sem_post(&rw->writePred);
	sem_post(&rw->writeCount);
}

void WriterUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the writer.
	sem_post(&rw->writeLock);
}
