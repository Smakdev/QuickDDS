#include<string>
#include<cinttypes>
#include<fstream>
#include<iostream>
#include"DDSHelper.h"

// FOR CREATING DIR
#ifdef _WIN32
#include<windows.h>
#endif // _WIN32


char gFCC;

bool ReadDDS(std::ifstream& stream)
{
	DDS_HEADER header{};
	stream.read((char*)&header, sizeof(DDS_HEADER));
	if (header.pf.FourCC == 0x30315844)
	{
		DDS_HEADER_DXT10 header10{};
		stream.read((char*)&header10, sizeof(DDS_HEADER_DXT10));
	}
	//printf("MipMapCount: %d\n", header.MipMapCount);
	if (header.pf.FourCC == 0x31545844)
	{
		//printf("DXT1");
		gFCC = '1';
		for (int i = 0; i < header.MipMapCount + 1; i++)
		{
			if (header.Width == 0 && header.Height != 0)
				header.Width = 1;
			if (header.Height == 0 && header.Width != 0)
				header.Height = 1;
			//printf("MipMap %d x %d\n", header.Height, header.Width);
			int TWidth = ((header.Width + 3) / 4) * 4;
			int THeight = ((header.Height + 3) / 4) * 4;
			float size = (TWidth * THeight);
			int chunkC = size / 16.0f + 0.9999;
			//printf("Size %d\n", chunkC * 8);
			stream.seekg(chunkC * 8, stream.cur);

			header.Width /= 2;
			header.Height /= 2;
		}
	}
	else if (
		header.pf.FourCC == 0x35545844 ||
		header.pf.FourCC == 0x34545844 ||
		header.pf.FourCC == 0x33545844 ||
		header.pf.FourCC == 0x32545844)
	{
		//printf("DXT%c", );
		gFCC = header.pf.FourCC >> 24 & 0xFF;
		for (int i = 0; i < header.MipMapCount + 1; i++)
		{
			if (header.Width == 0 && header.Height != 0)
				header.Width = 1;
			if (header.Height == 0 && header.Width != 0)
				header.Height = 1;
			//printf("MipMap: %d x %d\n", header.Height, header.Width);
			int RoundWidth = ((header.Width + 3) / 4) * 4;
			int RoundHeight = ((header.Height + 3) / 4) * 4;
			int chunkC = (RoundWidth / 4) * (RoundHeight / 4);
			//printf("Block: %d\n", chunkC);
			//printf("Size: %d\n", chunkC * 16);
			stream.seekg(chunkC * 16, stream.cur);

			header.Width /= 2;
			header.Height /= 2;
		}
	}
	else
	{
		printf("Unknown type %x\n", header.pf.FourCC);
		std::cin.get();
		return false;
	}

	return true;
}

void replaceslash(std::string& input)
{
	for (int i = 0; i < input.size() - 1; i++)
	{
		if (input[i] == '\\')
		{
			input[i] = '/';
		}
	}
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Not enough arguments.\n");
		printf("Usage is: QuickDDS File (WINDOWS ONLY)\n");
		printf("Usage is: QuickDDS File Destination\n");
		std::cin.get();
		return 1;
	}
	std::string srcFile = argv[1];
	std::string resDir = " ";

	printf("\nQuickDDS\n");
	printf("A simple DDS extraction tool, developed by SmakDev\n");
	printf("\nDestination: %s\n", resDir.c_str());

	if (argc == 3)
	{
		resDir = argv[2];

		for (int i = 0; i < resDir.size(); i++)
		{
			if (resDir[i] == '\\')
			{
				resDir[i] = '/';
			}
		}
		resDir += '/';
	}
#ifdef WIN32
	else if (argc == 2)
	{
		std::string Name = srcFile.substr(srcFile.find_last_of('\\') + 1); // GET FILE NAME WITHOUT EXTENSION
		Name = Name.substr(0, Name.find_last_of('.'));
		std::string Path = srcFile.substr(0, srcFile.find_last_of('\\')) + '\\'; // GET FILES FOLDER
		resDir = Path + Name + '\\'; // SET RESDIR TO DIRECTORY
		printf("Source: %s\n\n", srcFile.c_str());

		if (!CreateDirectoryA(resDir.c_str(), NULL))
		{
			int error = GetLastError();
			if (error == 0xB7) // PATH EXISTS
			{
				printf("\x1B[31mError %d: The directory already exists.\033[0m\n", error);
			}
			else
			{
				printf("\x1B[31mError: %d\033[0m\n", error);
			}
			printf("\nhttps://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes");
			std::cin.get();
			return -1;
		}
	}
#else
	if (argc == 2)
	{
		printf("Not enough arguments.\n");
		printf("Usage is: QuickDDS File (WINDOWS ONLY)\n");
		printf("Usage is: QuickDDS File Destination\n");
		std::cin.get();
		return 1;
	}
#endif // _WIN32

	printf("\nDestination: %s\n", resDir.c_str());
	printf("Source: %s\n\n", srcFile.c_str());
	printf("  Offset           Size          Name        FourCC\n");
	printf("---------------------------------------------------\n");

	std::ifstream iStream(srcFile.c_str(), std::ios::binary | std::ios::ate);
	if (!iStream.is_open())
	{
		printf("Could not open %s", srcFile.c_str());
		std::cin.get();
		return 1;
	}
	uint64_t FileSize = iStream.tellg();
	iStream.seekg(0, iStream.beg);

	int position = 0 ;
	uint32_t ResCount = 0;
	uint32_t NavByte = 0;
	for (uint64_t i = 0; i < FileSize; i++)
	{
		iStream.read((char*)&NavByte, 1);
		if ((NavByte & 0xFF) == 'D')
		{
			iStream.read(((char*)&NavByte) + 1, 3);
			if (NavByte != DDSMagic)
			{
				iStream.seekg(i + 1, iStream.beg);
				continue;
			}
			position = (unsigned long)iStream.tellg() - 4;
			bool good = ReadDDS(iStream);
			if (good)
			{
				ResCount++;
				uint64_t size = ((uint64_t)iStream.tellg()) - i;
				iStream.seekg(i, iStream.beg);
				std::string filename = resDir + std::to_string(ResCount) + ".dds";

				std::ofstream OStream(filename.c_str(), std::ios::binary);
				if (!OStream.is_open())
				{
					printf("\nCould not write to %s", filename.c_str());
					std::cin.get();
					return -1;
				}
				char* buffer = new char[size];
				iStream.read(buffer, size);
				OStream.write(buffer, size);
				OStream.close();
				delete[] buffer;
				printf("  %016x ", position);
				printf("%-*d", 11, (unsigned long)size);
				printf("%*d.dds        ", 4, ResCount);
				printf("DXT%c\n", gFCC);
				i = iStream.tellg();
				i--;
			}
			else
			{
				iStream.seekg(i + 1, iStream.beg);
			}
		}
	}
	iStream.close();
	printf("\n---------------------------------------------------\n");
	if (ResCount)
	{
		printf("Found %d DDS files\n", ResCount);
	}
	else
	{
		printf("No DDS Files found in file\n");
#ifdef _WIN32
		RemoveDirectoryA(resDir.c_str());
#endif // _WIN32
	}
	std::cin.get();
	return 0;
}
