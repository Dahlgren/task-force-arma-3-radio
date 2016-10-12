#pragma once
#include <ts3_functions.h>
#include <string>
#include <vector>
#include "server_radio_data.hpp"
#include <Windows.h>
int constexpr const_strlen(const char* str) {
	return *str ? 1 + const_strlen(str + 1) : 0;
}
class helpers {
public:
	static void applyGain(short * samples, int channels, size_t sampleCount, float directTalkingVolume);
	static void applyILD(short * samples, int channels, size_t sampleCount, TS3_VECTOR position, float viewAngle);
	static float sq(float x);
	static float distance(TS3_VECTOR from, TS3_VECTOR to) ;
	static float parseArmaNumber(const std::string& armaNumber);
	static int parseArmaNumberToInt(const std::string& armaNumber);
	static bool startsWith(const std::string& shouldStartWith, const std::string& startIn);
	static std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);
	static std::vector<std::string> split(const std::string& s, char delim);
	static bool isTrue(std::string& string);
	static short* allocatePool(int sampleCount, int channels, short* samples);
	static void mix(short* to, short* from, int sampleCount, int channels);
	static float volumeMultiplifier(const float volumeValue);
	static std::map<std::string, FREQ_SETTINGS> parseFrequencies(const std::string& string);
	static float clamp(float x, float a, float b);
	static std::pair<std::string, float> getVehicleDescriptor(std::string vechicleId);
	//String of format [0.123,0.123,0.123]
	static std::vector<float> coordStringToVector(const std::string& coordinateString) {
		std::vector<float> output;
		if (coordinateString.length() > 2) {
			std::vector<std::string> coords = helpers::split(coordinateString.substr(1, coordinateString.length() - 2), ',');
			for (const std::string& coord : coords)
				output.push_back(parseArmaNumber(coord));
		}
		return output;
	}
	static float volumeFromDistance(float distance, bool shouldPlayerHear, float clientDistance, float multiplifer = 1.0f) {
		if (distance <= 1.0) return 1.0;
		float maxDistance = shouldPlayerHear ? clientDistance * multiplifer : CANT_SPEAK_DISTANCE;
		float gain = powf(distance, -0.3f) * (max(0, (maxDistance - distance)) / maxDistance);
		if (gain < 0.001f) return 0.0f; else return min(1.0f, gain);
	}
	static float volumeFromDistance(float distance, bool shouldPlayerHear, int clientDistance, float multiplifer = 1.0f) {
		if (distance <= 1.0) return 1.0;
		float maxDistance = shouldPlayerHear ? static_cast<float>(clientDistance) * multiplifer : CANT_SPEAK_DISTANCE;
		float gain = powf(distance, -0.3f) * (max(0, (maxDistance - distance)) / maxDistance);
		if (gain < 0.001f) return 0.0f; else return min(1.0f, gain);
	}

	template<class T>	  //#MAYBE audioHelpers?
	static void processFilterStereo(short * samples, int channels, size_t sampleCount, float gain, T* filter) {
		for (size_t i = 0; i < sampleCount * channels; i += channels) {
			// all channels mixed
			float mix[MAX_CHANNELS];
			for (int j = 0; j < MAX_CHANNELS; j++) mix[j] = 0.0;

			// prepare mono for radio
			short to_process[MAX_CHANNELS];
			for (int j = 0; j < MAX_CHANNELS; j++) to_process[j] = 0;

			for (int j = 0; j < channels; j++) {
				to_process[j] = samples[i + j];
			}

			// process radio filter
			EnterCriticalSection(&serverDataCriticalSection);
			for (int j = 0; j < channels; j++) {
				floatsSample[j][0] = ((float) to_process[j] / (float) SHRT_MAX);
			}
			// skip other channels
			for (int j = channels; j < MAX_CHANNELS; j++) {
				floatsSample[j][0] = 0.0;
			}
			filter->process<float>(1, floatsSample);
			for (int j = 0; j < channels; j++) {
				mix[j] = floatsSample[j][0] * gain;
			}
			LeaveCriticalSection(&serverDataCriticalSection);

			// put mixed output to stream		
			for (int j = 0; j < channels; j++) {
				float sample = mix[j];
				short newValue;
				if (sample > 1.0) newValue = SHRT_MAX;
				else if (sample < -1.0) newValue = SHRT_MIN;
				else newValue = (short) (sample * (SHRT_MAX - 1));
				samples[i + j] = newValue;
			}
		}
	}

};

class CriticalSectionLock {
	CRITICAL_SECTION* cs;
public:
	explicit CriticalSectionLock(CRITICAL_SECTION* _cs) : cs(_cs) { EnterCriticalSection(cs); }
	~CriticalSectionLock() { LeaveCriticalSection(cs); }
};
#include <chrono>
class speedTest {
public:
	speedTest(const std::string & name_,bool willPrintOnDestruct_=true) :start(std::chrono::high_resolution_clock::now()), name(name_), willPrintOnDestruct(willPrintOnDestruct_){}
	~speedTest() { if (willPrintOnDestruct) log(); }
	void log() const {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
		log_string(name+" " + std::to_string(duration)+" microsecs", LogLevel_WARNING);
	}
	void log(const std::string & text) {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
		log_string(name + "-" + text + " " + std::to_string(duration) + " microsecs", LogLevel_WARNING);
		start += std::chrono::high_resolution_clock::now() - now; //compensate time for call to log func
	}
	void reset() {
		start = std::chrono::high_resolution_clock::now();
	}
	std::chrono::microseconds getCurrentElapsedTime() const {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(now - start);
	}
	std::chrono::high_resolution_clock::time_point start;
	std::string name;
	bool willPrintOnDestruct;
};