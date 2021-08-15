#pragma once
#include "imgui.h"
#include "imgui_memory_editor.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <filesystem>
#include "../Cpu/Arm.h"
#include "../Memory/MemoryBus.h"
#include "tinyfiledialogs.h"

struct DebugUI {
	DebugUI(sf::RenderWindow *window, MemoryBus *mbus, Arm *cpu);
	void render();
	void renderRegisters();
	void renderButtons();
	void renderMenuBar();
	void renderCartInfo();
	void renderPipeline();
	void renderDisplay();
	void update();
	void handleButtonPresses();
	void handleEvents(sf::Event& ev);

	static MemoryEditor mainMemory;
	static MemoryEditor biosMemory;
	static MemoryEditor gamepakMemory;

	bool showRegisterWindow;
	bool showBiosMemory;
	bool showGamePakMemory;
	bool showCartWindow;
	bool showPPUWindow;
	bool showPipeline;
	bool showDisplay;
	bool vsync;

	sf::RenderWindow* window;
	MemoryBus* mbus;
	Arm* cpu;
};
