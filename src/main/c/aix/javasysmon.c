/*
 *  javasysmon.c
 *  javanativetools
 *
 *  Created by Rich Coe on 17 Oct 2015.
 *  Copyright 2015 Jez Humble. All rights reserved.
 *  Licensed under the terms of the New BSD license.
 *  Thanks to: http://getthegood.com/TechNotes/Papers/ProcStatistics.html
 */
#include "javasysmon.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <sys/procfs.h>
#include <pwd.h>

#define MAXSTRSIZE 80

static int num_cpus;
static int pagesize;
static unsigned long long phys_mem;

long 
timespec_to_millisecs(pr_timestruc64_t time) 
{
	return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

JNIEXPORT jint JNICALL 
JNI_OnLoad(JavaVM * vm, void * reserved)
{
  num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  pagesize = sysconf(_SC_PAGESIZE);
  phys_mem = sysconf(_SC_PHYS_PAGES) * pagesize;

  return JNI_VERSION_1_2;
}

JNIEXPORT jobject JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_cpuTimes(JNIEnv *env, jobject obj)
{
  int i;
  unsigned long long userticks, systicks, idleticks;
  jclass		cpu_times_class;
  jmethodID	cpu_times_constructor;
  jobject		cpu_times;

  idleticks = systicks = userticks = 0; 

  cpu_times_class = (*env)->FindClass(env, "com/jezhumble/javasysmon/CpuTimes");
  cpu_times_constructor = (*env)->GetMethodID(env, cpu_times_class, "<init>", "(JJJ)V");
  cpu_times = (*env)->NewObject(env, cpu_times_class, cpu_times_constructor, (jlong) userticks, (jlong) systicks, (jlong) idleticks);
  (*env)->DeleteLocalRef(env, cpu_times_class);
  return cpu_times;
}


JNIEXPORT jobject JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_physical(JNIEnv *env, jobject obj)
{
  jclass		memory_stats_class;
  jmethodID	memory_stats_constructor;
  jobject		memory_stats;
  unsigned long long free_mem;

  free_mem = sysconf(_SC_AVPHYS_PAGES) * pagesize;
  memory_stats_class = (*env)->FindClass(env, "com/jezhumble/javasysmon/MemoryStats");
  memory_stats_constructor = (*env)->GetMethodID(env, memory_stats_class, "<init>", "(JJ)V");
  memory_stats = (*env)->NewObject(env, memory_stats_class, memory_stats_constructor, (jlong) free_mem, (jlong) phys_mem);
  (*env)->DeleteLocalRef(env, memory_stats_class);
  return memory_stats;
}

int 
get_swap_stats(unsigned long long *total_swap, unsigned long long *free_swap)
{
}

JNIEXPORT jobject JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_swap(JNIEnv *env, jobject obj)
{
  jclass		memory_stats_class;
  jmethodID	memory_stats_constructor;
  jobject		memory_stats;
  unsigned long long total_swap, free_swap;
  total_swap = free_swap = 0;
  get_swap_stats(&total_swap, &free_swap);

  memory_stats_class = (*env)->FindClass(env, "com/jezhumble/javasysmon/MemoryStats");
  memory_stats_constructor = (*env)->GetMethodID(env, memory_stats_class, "<init>", "(JJ)V");
  memory_stats = (*env)->NewObject(env, memory_stats_class, memory_stats_constructor, (jlong) free_swap, (jlong) total_swap);
  (*env)->DeleteLocalRef(env, memory_stats_class);
  return memory_stats;
}

JNIEXPORT jint JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_numCpus(JNIEnv *env, jobject obj)
{
    return (jint) num_cpus;
}

JNIEXPORT jlong JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_cpuFrequencyInHz(JNIEnv *env, jobject obj)
{
    return 0;
}

JNIEXPORT jlong JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_uptimeInSeconds(JNIEnv *env, jobject obj)
{
  struct timeval secs;
  unsigned long long uptime;

  if (gettimeofday(&secs, NULL) != 0) {
    return (jlong) 0;
  }
  uptime = (unsigned long long) secs.tv_sec;

  return 0;
}

JNIEXPORT jint JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_currentPid(JNIEnv *env, jobject obj)
{
  return (jint) getpid();
}

JNIEXPORT jobject JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_psinfoToProcess(JNIEnv *env, jobject object, jbyteArray psinfo, jbyteArray prstatus)
{
  jclass	process_info_class;
  jmethodID	process_info_constructor;
  jobject	process_info;
  psinfo_t      *info;
  pstatus_t     *usage;
  struct passwd	*user;

  process_info_class = (*env)->FindClass(env, "com/jezhumble/javasysmon/ProcessInfo");
  process_info_constructor = (*env)->GetMethodID(env, process_info_class, "<init>",
						 "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJJ)V");
  info = (psinfo_t*) (*env)->GetByteArrayElements(env, psinfo, NULL);
  usage = (pstatus_t*) (*env)->GetByteArrayElements(env, prstatus, NULL);
  user = getpwuid(info->pr_uid);
  // when somebody wants to get the command line, the trick is to get info->pr_argc (argument count)
  // and info->pr_argv (pointer to initial argument vector) and use it as an offset into /proc/<pid>/as
  process_info = (*env)->NewObject(env, process_info_class, process_info_constructor,
			       (jint) info->pr_pid,
			       (jint) info->pr_ppid,
			       (*env)->NewStringUTF(env, ""),
			       (*env)->NewStringUTF(env, info->pr_fname),
			       (*env)->NewStringUTF(env, user->pw_name),
			       (jlong) timespec_to_millisecs(usage->pr_utime),
			       (jlong) timespec_to_millisecs(usage->pr_stime),
			       (jlong) info->pr_rssize * 1024,
			       (jlong) info->pr_size * 1024);
  (*env)->ReleaseByteArrayElements(env, psinfo, (jbyte*) info, 0);
  (*env)->ReleaseByteArrayElements(env, prstatus, (jbyte*) usage, 0);
  (*env)->DeleteLocalRef(env, process_info_class);
  return process_info;
}

JNIEXPORT void JNICALL 
Java_com_jezhumble_javasysmon_AixMonitor_killProcess(JNIEnv *env, jobject object, jint pid) {
  kill(pid, SIGTERM);
}
