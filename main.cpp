/*
*
* Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2, as 
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
*/

#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <alsa/asoundlib.h>
#include <string> 
#include <queue>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "InputDevice.h"
#include "MediaPlayer.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <linux/input.h>
#include <linux/uinput.h>

#include <linux/kd.h>



bool isRunning = true;

std::vector<InputDevicePtr> inputDevices;
std::vector<std::string> devices;


/*
# Disable alpha layer (hide video)
echo "d01068b4 0x7fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr

# Enable alpha layer (show video)
echo "d01068b4 0x3fc0" | sudo tee /sys/kernel/debug/aml_reg/paddr
*/


// Signal handler for Ctrl-C
void SignalHandler(int s)
{
	isRunning = false;
}


void GetDevices()
{
	inputDevices.clear();


	DIR *dpdf;
	struct dirent *epdf;

	std::string path = "/dev/input/";

	dpdf = opendir(path.c_str());
	if (dpdf != NULL)
	{
		while (epdf = readdir(dpdf))
		{
			//printf("Filename: %s\n", epdf->d_name);
			// std::cout << epdf->d_name << std::endl;
			
			std::string entryName = path + std::string(epdf->d_name);
			//printf("Filename: %s\n", entryName.c_str());

			struct stat sb;
			if (stat(entryName.c_str(), &sb) == -1) {
				perror("stat");
				exit(EXIT_FAILURE);
			}

			//printf("File type:");

			//switch (sb.st_mode & S_IFMT) {
			//case S_IFBLK:  printf("block device\n");            break;
			//case S_IFCHR:  printf("character device\n");        break;
			//case S_IFDIR:  printf("directory\n");               break;
			//case S_IFIFO:  printf("FIFO/pipe\n");               break;
			//case S_IFLNK:  printf("symlink\n");                 break;
			//case S_IFREG:  printf("regular file\n");            break;
			//case S_IFSOCK: printf("socket\n");                  break;
			//default:       printf("unknown?\n");                break;
			//}

			//printf("\n");

			if (sb.st_mode & S_IFCHR)
			{
				devices.push_back(entryName);
				printf("added device: %s\n", entryName.c_str());
			}
		}

		for(auto device : devices)
		{
			printf("Device: %s\n", device.c_str());

			int fd = open(device.c_str(), O_RDONLY);
			if (fd < 0)
				throw Exception("device open failed.");

			//input_id id;
			//int io = ioctl(fd, EVIOCGID, &id);
			//if (io < 0)
			//{
			//	printf("EVIOCGID failed.\n");
			//}
			//else
			//{
			//	printf("\tbustype=%d, vendor=%d, product=%d, version=%d\n",
			//		id.bustype, id.vendor, id.product, id.version);
			//}

			//char name[255];
			//io = ioctl(fd, EVIOCGNAME(255), name);
			//if (io < 0)
			//{
			//	printf("EVIOCGNAME failed.\n");
			//}
			//else
			//{
			//	printf("\tname=%s\n", name);
			//}

			
			int bitsRequired = EV_MAX;
			int bytesRequired = (bitsRequired + 1) / 8;

			unsigned char buffer[bytesRequired];
			int io = ioctl(fd, EVIOCGBIT(0, bytesRequired), buffer);
			if (io < 0)
			{
				printf("EVIOCGBIT failed.\n");
			}
			else
			{
				unsigned int events = *((unsigned int*)buffer);

				if (events & EV_KEY)
				{
					InputDevicePtr inputDevice = std::make_shared<InputDevice>(device);
					inputDevices.push_back(inputDevice);
				}
			}
			
			close(fd);
		}
	}
}

struct option longopts[] = {
	{ "time",         required_argument,  NULL,          't' },
	{ "chapter",      required_argument,  NULL,          'c' },
	{ 0, 0, 0, 0 }
};


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		// TODO: Usage
		printf("TODO: Usage\n");
		return 0;
	}


	// options
	int c;
	double optionStartPosition = 0;
	int optionChapter = -1;

	while ((c = getopt_long(argc, argv, "t:c:", longopts, NULL)) != -1)
	{
		switch (c)
		{
		case 't':
		{
			if (strchr(optarg, ':'))
			{
				unsigned int h;
				unsigned int m;
				double s;
				if (sscanf(optarg, "%u:%u:%lf", &h, &m, &s) == 3)
				{
					optionStartPosition = h * 3600 + m * 60 + s;
				}
				else
				{
					printf("invalid time specification.\n");
					throw Exception();
				}
			}
			else
			{
				optionStartPosition = atof(optarg);
			}

			printf("startPosition=%f\n", optionStartPosition);
		}
		break;

		case 'c':
			optionChapter = atoi(optarg);
			printf("optionChapter=%d\n", optionChapter);
			break;

		default:
			throw NotSupportedException();

			//printf("?? getopt returned character code 0%o ??\n", c);
			//break;
		}
	}


	const char* url = nullptr;
	if (optind < argc)
	{
		//printf("non-option ARGV-elements: ");
		while (optind < argc)
		{
			url = argv[optind++];
			//printf("%s\n", url);
			break;
		}
	}


	if (url == nullptr)
	{
		// TODO: Usage
		printf("TODO: Usage\n");
		return 0;
	}


#if 1
	GetDevices();

	for (InputDevicePtr dev : inputDevices)
	{
		printf("Using input device: %s\n", dev->Name().c_str());
	}

#endif

	
	// Initialize libav
	av_log_set_level(AV_LOG_VERBOSE);
	av_register_all();
	avformat_network_init();


	MediaPlayerSPTR mediaPlayer = std::make_shared<MediaPlayer>(url);


	if (optionChapter > -1)
	{
		if (optionChapter <= mediaPlayer->Chapters()->size())
		{
			optionStartPosition = mediaPlayer->Chapters()->at(optionChapter - 1).Start;
			printf("MAIN: Chapter found (%f).\n", optionStartPosition);
		}
	}

	
	mediaPlayer->Seek(optionStartPosition);
	mediaPlayer->SetState(MediaState::Play);


	isRunning = true;
	bool isPaused = false;

	while (isRunning)
	{
		// Process Input
		int keycode;
		double newTime;

		for (InputDevicePtr dev : inputDevices)
		{
			while (dev->TryGetKeyPress(&keycode))
			{
				double currentTime = mediaPlayer->Position();

				switch (keycode)
				{
				case KEY_HOME:	// odroid remote
				case KEY_MUTE:
				case KEY_MENU:

				case KEY_VOLUMEDOWN:
				case KEY_VOLUMEUP:
					break;

				case KEY_POWER:	// odroid remote
				case KEY_BACK:
				case KEY_ESC:
				{
					isRunning = false;
				}
					break;

				case KEY_FASTFORWARD:
					//TODO
					break;

				case KEY_REWIND:
					//TODO
					break;

				case KEY_ENTER:	// odroid remote
				case KEY_SPACE:
				case KEY_PLAYPAUSE:
				{
					if (isPaused)
					{
						mediaPlayer->SetState(MediaState::Play);
					}
					else
					{
						mediaPlayer->SetState(MediaState::Pause);
					}
					
					isPaused = !isPaused;
				}
				break;

				case KEY_LEFT:
					// Backward
					if (!isPaused)
					{
						newTime = currentTime - 30.0; 
						goto seek;
					}
					break;

				case KEY_RIGHT:
					// Forward
					if (!isPaused)
					{
						newTime = currentTime + 30.0; 		
						goto seek;
					}
					break;

				case KEY_UP:
					if (!isPaused)
					{
						newTime = currentTime - 5.0 * 60; 
						goto seek;
					}
					break;

				case KEY_DOWN:
					newTime = currentTime + 5.0 * 60; 

seek:
					if (!isPaused)
					{			
						printf("Seeking from %f to %f.\n", currentTime, newTime);

						mediaPlayer->Seek(newTime);
					}
					break;

				default:
					break;
				}
			}
		}


		if (mediaPlayer->IsEndOfStream())
		{
			isRunning = false;
		}
		else
		{
			usleep(1000);
		}
	}


	printf("MAIN: Playback finished.\n");

	return 0;
}