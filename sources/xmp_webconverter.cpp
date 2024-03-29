#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <zlib.h>
#include <string>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <CUBEParser.h>
#include <emscripten/bind.h>

typedef std::string string;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

struct free_delete
{
	void operator()(void *ptr) const { free(ptr); }
};

string md5_process(uint8 *, size_t);

int32 int_round(double n) { return n >= 0 ? (int32)(n + .5) : (int32)(n - .5); }

void shrink(double *to_shrink, uint16 *shrinked, int32 size, int32 new_size)
{
	/* fast and easy tetrahedral interpolation rounded for uint16,
	source: https://www.nvidia.com/content/GTC/posters/2010/V01-Real-Time-Color-Space-Conversion-for-High-Resolution-Video.pdf
	I've made some casts in order to produce exactly the same values of Camera Raw, note, few times when ratio have many decimals(like size 36 --> 28)
	could be differ of (1/65535)*255 due to math of compiler, obv is so little that doesn't affect quality(or similarity with Lightroom) */

	const double ratio = (size - 1.0) / (new_size - 1.0);
	const int32 size2 = size * size;
	for (int32 i = 0, idx; i < new_size; ++i)
		for (int32 j = 0; j < new_size; ++j)
			for (int32 k = 0; k < new_size; ++k)
			{
				int32 lr = std::clamp((int32)(i * ratio), 0, size - 1);
				int32 lg = std::clamp((int32)(j * ratio), 0, size - 1);
				int32 lb = std::clamp((int32)(k * ratio), 0, size - 1);
				int32 ur = std::clamp(lr + 1, 0, size - 1);
				int32 ug = std::clamp(lg + 1, 0, size - 1);
				int32 ub = std::clamp(lb + 1, 0, size - 1);
				double fR = (double)i * ratio - lr;
				double fG = (double)j * ratio - lg;
				double fB = (double)k * ratio - lb;

				idx = (i * new_size * new_size + j * new_size + k) * 3;
				if (fG >= fB && fB >= fR)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fG) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fG - fB) * (int_round((float)to_shrink[(lr * size2 + ug * size + lb) * 3 + l] * 65535)) + (fB - fR) * (int_round((float)to_shrink[(lr * size2 + ug * size + ub) * 3 + l] * 65535)) + fR * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));

				else if (fB > fR && fR > fG)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fB) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fB - fR) * (int_round((float)to_shrink[(lr * size2 + lg * size + ub) * 3 + l] * 65535)) + (fR - fG) * (int_round((float)to_shrink[(ur * size2 + lg * size + ub) * 3 + l] * 65535)) + fG * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));

				else if (fB > fG && fG >= fR)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fB) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fB - fG) * (int_round((float)to_shrink[(lr * size2 + lg * size + ub) * 3 + l] * 65535)) + (fG - fR) * (int_round((float)to_shrink[(lr * size2 + ug * size + ub) * 3 + l] * 65535)) + fR * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));

				else if (fR >= fG && fG > fB)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fR) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fR - fG) * (int_round((float)to_shrink[(ur * size2 + lg * size + lb) * 3 + l] * 65535)) + (fG - fB) * (int_round((float)to_shrink[(ur * size2 + ug * size + lb) * 3 + l] * 65535)) + fB * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));

				else if (fG > fR && fR >= fB)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fG) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fG - fR) * (int_round((float)to_shrink[(lr * size2 + ug * size + lb) * 3 + l] * 65535)) + (fR - fB) * (int_round((float)to_shrink[(ur * size2 + ug * size + lb) * 3 + l] * 65535)) + fB * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));

				else if (fR >= fB && fB >= fG)
					for (uint32 l = 0; l < 3; ++l)
						shrinked[idx + l] = 0.5 + (float)((1 - fR) * (int_round((float)to_shrink[(lr * size2 + lg * size + lb) * 3 + l] * 65535)) + (fR - fB) * (int_round((float)to_shrink[(ur * size2 + lg * size + lb) * 3 + l] * 65535)) + (fB - fG) * (int_round((float)to_shrink[(ur * size2 + lg * size + ub) * 3 + l] * 65535)) + fG * (int_round((float)to_shrink[(ur * size2 + ug * size + ub) * 3 + l] * 65535)));
			}
}

bool dirExists(const string &path)
{
	struct stat sb;
	if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
		return 1;
	else
		return 0;
}

bool fileExists(const string &filename)
{
	struct stat sb;
	if (stat(filename.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
		return 1;
	else
		return 0;
}

bool mkpath(string path)
{
	string dir;
	for (size_t pos = 0; pos != string::npos; pos = path.find_first_of("/\\", pos))
	{
		dir = path.substr(0, pos++);
		if (mkdir(dir.c_str(), 0755) == -1 && fileExists(dir))
			return 0;
	}
	return 1;
}

void read_directory(const string &name, std::vector<string> &v)
{
	DIR *dirp = opendir(name.c_str());
	struct dirent *dp;
	while ((dp = readdir(dirp)) != NULL)
	{
		char *check_extension;
		if ((check_extension = strrchr(dp->d_name, '.')))
		{
			string filename = dp->d_name;
			for (uint32 i = 0; i < strlen(check_extension); ++i)
				check_extension[i] = tolower(check_extension[i]);
			if (check_extension != NULL && ((strcmp(check_extension, ".cube") == 0) xor (strcmp(check_extension, ".xmp") == 0)))
				v.push_back(name + "/" + filename);
		}
	}
	closedir(dirp);
}

void zip_files(std::vector<string> &zip_list, string output)
{

	void *writer = NULL;
	mz_zip_writer_create(&writer);
	mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);
	mz_zip_writer_set_compress_level(writer, 9);

	if (mz_zip_writer_open_file(writer, output.c_str(), 0, false) == MZ_OK)
	{
		for (const auto &file : zip_list)
			if (mz_zip_writer_add_path(writer, file.c_str(), NULL, false, false) == MZ_OK)
			{
				if (remove(file.c_str()) != 0)
					printf("remove error\n");
			}
	}

	if (mz_zip_writer_close(writer) != MZ_OK)
		printf("Error closing archive for writing\n");
	mz_zip_writer_delete(&writer);
}

static struct defaults
{
	string gamut;
	int32 size = 32;
	string outFileName;
	uint32 min = 0;
	uint32 max = 200;
	uint32 amount = 100;
	string primaries;
	string strength;
	string inputFiles_list;
	string title;
	string group;
} options;

static string xmp_container[] = {"<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Adobe XMP Core 7.0-c000 1.000000, 0000/00/00-00:00:00        \">\n <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n  <rdf:Description rdf:about=\"\"\n    xmlns:crs=\"http://ns.adobe.com/camera-raw-settings/1.0/\"\n   crs:PresetType=\"Look\"\n   crs:Cluster=\"\"\n   crs:UUID=\"", "\"\n   crs:SupportsAmount=\"True\"\n   crs:SupportsColor=\"True\"\n   crs:SupportsMonochrome=\"True\"\n   crs:SupportsHighDynamicRange=\"True\"\n   crs:SupportsNormalDynamicRange=\"True\"\n   crs:SupportsSceneReferred=\"True\"\n   crs:SupportsOutputReferred=\"True\"\n   crs:RequiresRGBTables=\"False\"\n   crs:CameraModelRestriction=\"\"\n   crs:Copyright=\"\"\n   crs:ContactInfo=\"\"\n   crs:Version=\"14.3\"\n   crs:ProcessVersion=\"11.0\"\n", "   crs:ConvertToGrayscale=\"False\"\n   crs:RGBTable=\"", "\"\n   crs:Table_", "=\"", "\"\n   crs:HasSettings=\"True\">\n   <crs:Name>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\">", "</rdf:li>\n    </rdf:Alt>\n   </crs:Name>\n   <crs:ShortName>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\"/>\n    </rdf:Alt>\n   </crs:ShortName>\n   <crs:SortName>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\"/>\n    </rdf:Alt>\n   </crs:SortName>\n   <crs:Group>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\"", "\n    </rdf:Alt>\n   </crs:Group>\n   <crs:Description>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\"/>\n    </rdf:Alt>\n   </crs:Description>\n  </rdf:Description>\n </rdf:RDF>\n</x:xmpmeta>\n"};

string get_file_contents(string filename)
{
	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		string contents;
		fseek(fp, 0, SEEK_END);
		contents.resize(ftell(fp));
		rewind(fp);
		fread(&contents[0], 1, contents.size(), fp);
		fclose(fp);
		return (contents);
	}
	throw;
}

bool encode(string path, string outFileName)
{
	char title[100] = {0};
	int32_t input_size;
	double *samples_1_rawptr = nullptr;
	int32_t result = CUBEParser(path.data(), 1, title, &input_size, &samples_1_rawptr);
	auto samples_1 = std::unique_ptr<double[], free_delete>(samples_1_rawptr);

	switch (result)
	{
	case -1:
		printf("Error - no data lut\n");
		result = 0;
		break;
	case -2:
		printf("Size error - not a CUBE file\n");
		result = 0;
		break;
	case 0:
		// printf("- test encoding back -\nTITLE: %s\nSIZE: %d\n", (options.title.empty()) ? title : options.title.c_str(), input_size);

		uint32 size = (input_size > options.size) ? options.size : input_size;
		// if(input_size>32) printf("ACR unsupports LUT>32, resampling enabled\n");
		auto nopValue_1 = std::make_unique<uint16[]>(size);
		for (uint32 index = 0; index < size; index++)
			nopValue_1[index] = (index * 0x0FFFF + (size >> 1)) / (size - 1);
		uint32 padding = 16 + size * size * size * 3 * 2 + 28;
		auto samples_2 = std::make_unique<uint8[]>(padding);

		uint32 header[4] = {1, 1, 3, size};
		memcpy(samples_2.get(), header, 16);
		uint32 colors = 0, gamma = 1; // default sRGB
		if (options.primaries == "Adobe")
		{
			colors = 1;
			gamma = 3;
		}
		else if (options.primaries == "ProPhoto")
		{
			colors = 2;
			gamma = 2;
		}
		else if (options.primaries == "P3")
		{
			colors = 3;
			gamma = 1;
		}
		else if (options.primaries == "Rec2020")
		{
			colors = 4;
			gamma = 4;
		}

		uint32 gamut = (options.gamut == "extend");

		uint32 footer[3] = {colors, gamma, gamut};
		memcpy(&samples_2[16 + size * size * size * 3 * 2], footer, 12);
		double range[2] = {options.min * 0.01, options.max * 0.01};
		memcpy(&samples_2[16 + size * size * size * 3 * 2 + 12], range, 16);

		if (input_size != size)
		{
			auto shrinked = std::make_unique<uint16[]>(size * size * size * 3);
			shrink(samples_1.get(), shrinked.get(), input_size, size);

			for (uint32 bIndex = 0, idx, j = 0; bIndex < size; ++bIndex)
				for (uint32 gIndex = 0; gIndex < size; ++gIndex)
					for (uint32 rIndex = 0; rIndex < size; ++rIndex, j += 3)
					{
						idx = 16 + (rIndex * size * size + gIndex * size + bIndex) * 3 * 2;

						uint16 temp = shrinked[j] - nopValue_1[rIndex];
						memcpy(&samples_2[idx], &temp, 2);

						temp = shrinked[j + 1] - nopValue_1[gIndex];
						memcpy(&samples_2[idx + 2], &temp, 2);

						temp = shrinked[j + 2] - nopValue_1[bIndex];
						memcpy(&samples_2[idx + 4], &temp, 2);
					}
		}
		else
		{
			for (uint32 bIndex = 0, idx, j = 0; bIndex < size; ++bIndex)
				for (uint32 gIndex = 0; gIndex < size; ++gIndex)
					for (uint32 rIndex = 0; rIndex < size; ++rIndex, j += 3)
					{
						idx = 16 + (rIndex * size * size + gIndex * size + bIndex) * 3 * 2;

						uint16 temp = (int_round(samples_1[j] * 65535)) - nopValue_1[rIndex];
						memcpy(&samples_2[idx], &temp, 2);

						temp = (int_round(samples_1[j + 1] * 65535)) - nopValue_1[gIndex];
						memcpy(&samples_2[idx + 2], &temp, 2);

						temp = (int_round(samples_1[j + 2] * 65535)) - nopValue_1[bIndex];
						memcpy(&samples_2[idx + 4], &temp, 2);
					}
		}

		// printf("tot: %d\n",padding);
#ifdef DEBUG
		FILE *f_5 = fopen("outputarray.txt", "wb");
		for (uint32 i = 0, j = 0, k = 0; i < padding; ++i)
		{
			if (i >= 16 && i < padding - 28)
			{
				if (j % 6 == 0)
				{
					fputs(("\n"), f_5);
					j = 0;
				}
				j++;
			}
			else if (i >= padding - 18)
			{
				if (j % 8 == 0)
				{
					fputs(("\n"), f_5);
					j = 0;
				}
				j++;
			}
			else
			{
				if (k == 4)
				{
					fputs(("\n"), f_5);
					k = 0;
				}
				k++;
			}
			fprintf(f_5, "%d ", samples_2[i]);
		}
		fclose(f_5);
#endif

		auto block1_1 = std::move(samples_2);
		uint32 uncompressedSize_1 = padding;
		uint32 safeCompressedSize = (uncompressedSize_1 | uncompressedSize_1 >> 8) + 64;
		// printf("\n%s %d \n%s %d\n","uncompressedSize_1:",uncompressedSize_1,"safeCompressedSize:",safeCompressedSize);

		string MD5 = md5_process(block1_1.get(), uncompressedSize_1);
		// printf("MD5: %s\n", MD5.c_str());
		string UUID = MD5 + std::to_string(time_t(time(NULL)));
		UUID = md5_process((uint8 *)UUID.c_str(), UUID.length());
		// printf("UUID(aka MD5 of MD5+TIME): %s\n",UUID.c_str());

		auto dPtr_1 = std::make_unique<uint8[]>(safeCompressedSize + 4);
		memcpy(dPtr_1.get(), &uncompressedSize_1, 4);
		uLongf dCount = safeCompressedSize;
		compress2(&dPtr_1[4], &dCount, block1_1.get(), uncompressedSize_1, Z_DEFAULT_COMPRESSION);
		// printf("%s %d\n","zResult_1:",zResult_1);
		uint32 compressedSize_1 = (uint32)dCount + 4;
#ifdef DEBUG
		FILE *f_1 = fopen("outputencoded.txt", "wb");
		for (uint32 i = 0; i < safeCompressedSize + 4; ++i)
			fputs((std::to_string(dPtr_1[i]) + " ").c_str(), f_1);
		fclose(f_1);
#endif

		// printf("%s %d\n","compressedSize_1:",compressedSize_1);
		uLongf destLen_1 = uncompressedSize_1;
		auto block3_1 = std::make_unique<uint8[]>(uncompressedSize_1);
		uncompress(block3_1.get(), &destLen_1, &dPtr_1[4], compressedSize_1 - 4);
		// printf("%s %d\n","zResult_2:",zResult_2);
#ifdef DEBUG
		FILE *f_2 = fopen("outputencoded_1.txt", "wb");
		for (uint32 i = 0; i < uncompressedSize_1; ++i)
			fputs((std::to_string(block3_1[i]) + " ").c_str(), f_2);
		fclose(f_2);
#endif

		static const char *kEncodeTable = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?`'|()[]{}@%$#";
		uint32 safeEncodedSize = compressedSize_1 + (compressedSize_1 >> 2) + (compressedSize_1 >> 6) + 16;

		auto sPtr_1 = std::move(dPtr_1);
		sPtr_1[compressedSize_1] = 0;
		sPtr_1[compressedSize_1 + 1] = 0;
		sPtr_1[compressedSize_1 + 2] = 0;
		auto dPtr_2 = std::make_unique<uint8[]>(safeEncodedSize);
		uint32 k = 0;

		const uint32 *sPtr_1_ = reinterpret_cast<const uint32 *>(sPtr_1.get());
		for (uint32 i = 0, x; compressedSize_1; ++i)
		{
			x = *(sPtr_1_ + i);
			for (uint32 j = 0; j < 5; ++j, x /= 85)
			{
				dPtr_2[k++] = kEncodeTable[x % 85];
				if (j > 0 && !--compressedSize_1)
					break;
			}
		}

		FILE *f_6 = fopen(outFileName.c_str(), "wb");
		if (f_6)
		{
			string assembled = xmp_container[0] + UUID + xmp_container[1] + options.strength + xmp_container[2] + MD5 + xmp_container[3] + MD5 + xmp_container[4];
			fwrite(assembled.c_str(), 1, assembled.size(), f_6);
			fwrite(dPtr_2.get(), 1, k, f_6);
			if (options.amount > 0 && options.amount <= 200 && options.amount != 100)
			{
				string val = std::to_string(options.amount * 0.01);
				val.erase(val.find_last_not_of('0') + 1, string::npos).erase(val.find_last_not_of('.') + 1, string::npos);
				assembled = "\"\n   crs:RGBTableAmount=\"" + val;
			}
			else
				assembled = "";
			string group = (options.group.empty()) ? "/>" : ">" + options.group + "</rdf:li>";
			assembled += (options.title.empty()) ? xmp_container[5] + title + xmp_container[6] + group + xmp_container[7] : xmp_container[5] + options.title + xmp_container[6] + group + xmp_container[7];
			fwrite(assembled.c_str(), 1, assembled.size(), f_6);
			fclose(f_6);
		}
		else
			printf("Error writing file\n");
		// printf("%s %d\n%s %d\n","safeEncodedSize:",safeEncodedSize,"true EncodedSize:",k);
		result = 1;
		break;
	}

	return result;
}

constexpr float INV_65535 = 1 / 65535.f;

bool decode(string path, string outFileName)
{
	uint32 compressedSize = 0;
	string block1 = get_file_contents(path);
	size_t found = block1.find("Name>\n    <rdf:Alt>\n     <rdf:li xml:lang=\"x-default\">") + 54;
	// if (found==53) found = block1.find("Name>\r\n    <rdf:Alt>\r\n     <rdf:li xml:lang=\"x-default\">")+56;
	string title = block1.substr(found, block1.find("</", found) - found);
	found = block1.find("=\"", block1.find(":RGBTable=\"") + 11) + 2;
	block1 = block1.substr(found, (block1.find("\"", found)) - found);

	if (block1.size() > 14)
	{

		uint32 encodedSize = (uint32)block1.length();

		uint32 maxCompressedSize = (encodedSize + 4) / 5 * 4;
		auto dPtr = std::make_unique<uint8[]>(maxCompressedSize);
		// printf("- test decoding -\n%s %d\n","maxCompressedSize:",maxCompressedSize);

		static const uint8 kDecodeTable[96] = {0xFF, 0x44, 0xFF, 0x54, 0x53, 0x52, 0xFF, 0x49, 0x4B, 0x4C, 0x46, 0x41, 0xFF, 0x3F, 0x3E, 0x45, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x40, 0xFF, 0xFF, 0x42, 0xFF, 0x47, 0x51, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x4D, 0xFF, 0x4E, 0x43, 0xFF, 0x48, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x4F, 0x4A, 0x50, 0xFF, 0xFF};

		uint32 phase = 0, value;

		for (const char *sPtr = block1.c_str(); *sPtr; sPtr++)
		{
			uint8 e = *sPtr;
			if (e < 32 || e > 127)
				continue;
			uint32 d = kDecodeTable[e - 32];
			if (d > 85)
				continue;
			phase++;
			switch (phase)
			{
			case 1:
				value = d;
				break;
			case 2:
				value += d * 85;
				break;
			case 3:
				value += d * (85 * 85);
				break;
			case 4:
				value += d * (85 * 85 * 85);
				break;
			case 5:
				value += d * (85 * 85 * 85 * 85);
				memcpy(&dPtr[compressedSize], &value, 4);
				compressedSize += 4;
				phase = 0;
				break;
			}
		}

		uint32 current_idx = compressedSize;
		// printf("phase: %d",phase);
		switch (phase)
		{
		case 4:
			dPtr[current_idx + 2] = value >> 16;
			compressedSize++;
		case 3:
			dPtr[current_idx + 1] = value >> 8;
			compressedSize++;
		case 2:
			dPtr[current_idx] = value;
			compressedSize++;
		}

		uint32 uncompressedSize = *(reinterpret_cast<uint32 *>(dPtr.get()));

		// printf("\n%s %d\n%s %d\n%s %d\n%s %d\n", "encodedSize:", encodedSize, "compressedSize:", compressedSize, "uncompressedSize:", uncompressedSize, "compressedSize-4:", compressedSize-4);
#ifdef DEBUG
		FILE *f = fopen("outputdecoded.txt", "wb");
		for (uint32 i = 0; i < compressedSize; ++i)
			fprintf(f, "%d ", dPtr[i]);
		fclose(f);
#endif

		auto block3 = std::make_unique<uint8[]>(uncompressedSize);
		uLongf destLen = uncompressedSize;
		uncompress(block3.get(), &destLen, &dPtr[4], compressedSize - 4);
		// printf("\n%s %d\n","zResult:",zResult);
#ifdef DEBUG
		FILE *f_3 = fopen("final.txt", "wb");
		for (uint32 i = 0, j = 0, k = 0; i < uncompressedSize; ++i)
		{
			if (i >= 16 && i < uncompressedSize - 28)
			{
				if (j % 6 == 0)
				{
					fputs(("\n"), f_3);
					j = 0;
				}
				j++;
			}
			else if (i >= uncompressedSize - 18)
			{
				if (j % 8 == 0)
				{
					fputs(("\n"), f_3);
					j = 0;
				}
				j++;
			}
			else
			{
				if (k == 4)
				{
					fputs(("\n"), f_3);
					k = 0;
				}
				k++;
			}
			fprintf(f_3, "%d ", block3[i]);
		}
		fclose(f_3);
#endif

		// printf("MD5: %s\n", md5_process(block3,uncompressedSize).c_str());

		const uint8 fDivisions = block3[12];
		auto nopValue = std::make_unique<uint16[]>(fDivisions);
		for (uint32 index = 0; index < fDivisions; index++)
			nopValue[index] = (index * 0x0FFFF + (fDivisions >> 1)) / (fDivisions - 1);

		FILE *f_4 = fopen(outFileName.c_str(), "wb");
		if (f_4)
		{
			fprintf(f_4, "TITLE \"%s\"\nDOMAIN_MIN 0 0 0\nDOMAIN_MAX 1 1 1\nLUT_3D_SIZE %d\n", title.c_str(), fDivisions);

			const uint16 *block3_ = reinterpret_cast<const uint16 *>(block3.get());
			for (uint32 rIndex = 0, idx; rIndex < fDivisions; ++rIndex)
				for (uint32 gIndex = 0; gIndex < fDivisions; ++gIndex)
					for (uint32 bIndex = 0; bIndex < fDivisions; ++bIndex)
					{
						idx = 8 + (rIndex + gIndex * fDivisions + bIndex * fDivisions * fDivisions) * 3;
						fprintf(f_4, "%.9f %.9f %.9f\n",
								(uint16)(block3_[idx + 0] + nopValue[bIndex]) * INV_65535,
								(uint16)(block3_[idx + 1] + nopValue[gIndex]) * INV_65535,
								(uint16)(block3_[idx + 2] + nopValue[rIndex]) * INV_65535);
					}
			fclose(f_4);
		}
		else
			printf("Error writing file\n");
		return 1;
	}
	else
		printf("not a valid xmp profile\n");
	return 0;
}

std::vector<string> split(string arguments, const string &delimiter)
{
	std::vector<string> output;
	for (size_t pos = 0; (pos = arguments.find(delimiter)) != string::npos; arguments.erase(0, pos + delimiter.length()))
	{
		output.push_back(arguments.substr(0, pos));
	}
	output.push_back(arguments);
	return output;
}

void options_setter(defaults args)
{

	options.amount = (args.amount > 0 && args.amount <= 200) ? args.amount : 100;
	options.size = (args.size > 1 && args.size <= 32) ? args.size : 32;
	options.min = (args.min >= 0 && args.min <= 100) ? args.min : 0;
	options.max = (args.max >= 100 && args.max <= 200) ? args.max : 200;
	options.primaries = args.primaries;
	options.gamut = args.gamut;
	options.outFileName = args.outFileName;
	options.title = args.title;
	options.group = args.group;

	if (args.strength == "medium")
		options.strength = "   crs:ToneMapStrength=\"1\"\n";
	else if (args.strength == "high")
		options.strength = "   crs:ToneMapStrength=\"2\"\n";
	else
		options.strength = "";

	std::vector<string> inputFiles;
	if (!args.inputFiles_list.empty())
		for (const auto &inputFile : split(args.inputFiles_list, "|"))
		{
			if (dirExists(inputFile))
				read_directory(inputFile + "/", inputFiles);
			else
				inputFiles.push_back(inputFile);
		}

	if (!inputFiles.empty())
	{

		if (!mkpath(options.outFileName))
			printf("can't write here\n");
		bool outputIsDir = false;
		if (dirExists(options.outFileName))
			outputIsDir = true;

		if (inputFiles.size() > 1)
		{
			if (!options.title.empty())
				options.title = "";
			if (!options.outFileName.empty() && !outputIsDir)
			{
				if (!mkpath(options.outFileName + "/"))
					printf("can't write output dir\n");
				outputIsDir = true;
			}
		}

		string ext[] = {"xmp", "cube"};
		std::vector<string> zip_list;
		for (const string &inputFile : inputFiles)
		{
			if (fileExists(inputFile))
			{
				string filecheck = inputFile.substr(inputFile.find_last_of(".") + 1);
				transform(filecheck.begin(), filecheck.end(), filecheck.begin(), tolower);

				string outFileName;
				if (filecheck == ext[0] xor filecheck == ext[1])
				{
					if (outputIsDir)
					{
						outFileName = options.outFileName + "/" + inputFile.substr(inputFile.find_last_of("/\\") + 1, inputFile.find_last_of(".") - (inputFile.find_last_of("/\\") + 1)) + ".";
						outFileName += (filecheck == ext[0]) ? ext[1] : ext[0];
					}
					else if (!options.outFileName.empty())
						outFileName = options.outFileName;
					else
					{
						outFileName = inputFile.substr(inputFile.find_last_of("/\\") + 1, inputFile.find_last_of(".") - (inputFile.find_last_of("/\\") + 1)) + ".";
						outFileName += (filecheck == ext[0]) ? ext[1] : ext[0];
					}
					bool result = (filecheck == ext[0]) ? decode(inputFile, outFileName) : encode(inputFile, outFileName);
					if (result)
						zip_list.push_back(outFileName);
				}
			}
		}
		if (zip_list.size() > 1)
			zip_files(zip_list, "converted_luts.zip");
	}
}

EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::value_object<defaults>("options_list")
		.field("size", &defaults::size)
		.field("min", &defaults::min)
		.field("max", &defaults::max)
		.field("amount", &defaults::amount)
		.field("primaries", &defaults::primaries)
		.field("gamut", &defaults::gamut)
		.field("strength", &defaults::strength)
		.field("title", &defaults::title)
		.field("group", &defaults::group)
		.field("input", &defaults::inputFiles_list)
		.field("output", &defaults::outFileName);

	emscripten::function("decode", &decode);
	emscripten::function("encode", &encode);
	emscripten::function("options", &options_setter);
}
