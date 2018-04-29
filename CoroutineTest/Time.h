#pragma once

class Time {
	inline static float S_DT;
	inline static float S_GameTime;

public:
	inline static float DT() { return S_DT; }
	inline static float GameTime() { return S_GameTime; }
	inline static float GameTimeSince(float EarlierTime) { return S_GameTime - EarlierTime; }

	inline static void Update(float DT) {
		S_DT = DT;
		S_GameTime += DT;
	}
};