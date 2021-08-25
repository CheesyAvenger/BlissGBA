#pragma once
#include "../Debugger/DebugUI.h"
#include "../Ppu/Ppu.h"

class Emulator {
public:
	Emulator(sf::RenderWindow *window, float displayScaleFactor);
	void run();
	void render(sf::RenderTarget &target);
	void reset();

	void handleEvents(sf::Event& ev);

	MemoryBus mbus;
	Ppu ppu;
	Arm cpu;
	DebugUI debug;

	bool showDebugger; //if false, emulator will render full screen
	bool running;
	const int scanlinesPerFrame = 228;
	const int maxCycles = (1232 * scanlinesPerFrame); //1232 cycles per scanline (308 dots * 4 cpu cycles)
	float displayScaleFactor;
};
