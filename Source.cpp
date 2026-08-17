#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "raylib.h"

#define Cell char
#define SizeOfCell sizeof(Cell)

#define AliveState (char)(255)
#define DeadState  (char)(0)

inline void LifeUpdate();
void LifeThreadLoop();

static Cell* World = NULL;
static Cell* ConstantWorld = NULL;

static int WorldLength = 64;
static int NeighbourhoodSize = 1;

static int Overpopulation = 3;
static int Birth = 3;
static int Underpopulation = 2;

static unsigned int Generation = 0;
static unsigned int GPS = 0;

static bool RunLife = false;

static bool Paused = false;

int main()
{
	{
		int editrules;

		std::cout << "== OPTIONS ==";
		std::cout << "\nHow big is the universe in cells : ";
		std::cin >> WorldLength;
		std::cout << "\nedit rules : ";
		std::cin >> editrules;

		if (editrules)
		{
			std::cout << "\n\n= RULES =";

			std::cout << "\nNeighbourhood size : ";
			std::cin >> NeighbourhoodSize;

			std::cout << "\nOverpopulation threshold : ";
			std::cin >> Overpopulation;

			std::cout << "\Birth count : ";
			std::cin >> Birth;

			std::cout << "\nUnderpopulation threshold : ";
			std::cin >> Underpopulation;
		}
	}

	std::cout << "\n\n== CONTROLLS ==";
	std::cout << "\n\'P\' = pause";
	std::cout << "\n\'=\' = increment";
	std::cout << "\n\'C\' = clear";
	std::cout << "\n\'R\' = clear random";
	std::cout << "\n\'Left click\' = set cell";
	std::cout << "\n\'Right click\' = set death\n\n";

	SetTraceLogLevel(4);

	if (NeighbourhoodSize < 1)
		return -1;

	if (WorldLength < 1)
		return -1;

	World = (Cell*)calloc(WorldLength * WorldLength * SizeOfCell, SizeOfCell);

	if (World == NULL)
		return -1;

	ConstantWorld = (Cell*)calloc(WorldLength * WorldLength * SizeOfCell, SizeOfCell);

	if (ConstantWorld == NULL)
		return -1;

	for (int x = 0; x < WorldLength; x++)
	{
		for (int y = 0; y < WorldLength; y++)
		{
			if (rand() % 3 == 0)
			{
				*(World + ((y * WorldLength) + x)) = AliveState;
			}
		}
	}

	{
		unsigned int windowwidth = 0;

		if (WorldLength < 256)
		{
			windowwidth = 256;
		}
		else
		{
			windowwidth = WorldLength;
		}

		InitWindow(windowwidth, WorldLength + 45, "life");
	}

	SetTargetFPS(60);

	{
		RunLife = true;
		std::thread lifeloop(LifeThreadLoop);

		bool pausekeydown = false;

		while (WindowShouldClose() == false)
		{
			if (IsMouseButtonDown(0))
			{
				*(World + ((((GetMouseY() - 1) % WorldLength) * WorldLength) + (GetMouseX() % WorldLength) * SizeOfCell)) = AliveState;
			}

			if (IsMouseButtonDown(1))
			{
				*(World + ((((GetMouseY() - 1) % WorldLength) * WorldLength) + (GetMouseX() % WorldLength) * SizeOfCell)) = DeadState;
			}

			if (IsKeyDown(KEY_R))
			{
				for (int x = 0; x < WorldLength; x++)
				{
					for (int y = 0; y < WorldLength; y++)
					{
						*(World + ((y * WorldLength) + x)) = DeadState;

						if (rand() % 3 == 0)
						{
							*(World + ((y * WorldLength) + x)) = AliveState;
						}
					}
				}
			}

			if (IsKeyDown(KEY_C))
			{
				for (int x = 0; x < WorldLength; x++)
				{
					for (int y = 0; y < WorldLength; y++)
					{
						*(World + ((y * WorldLength) + x)) = DeadState;
					}
				}
			}

			if (IsKeyDown(KEY_P))
			{
				if (pausekeydown == false)
				{
					pausekeydown = true;
					Paused = !Paused;
				}
			}
			else
			{
				pausekeydown = false;
			}

			BeginDrawing();

			ClearBackground({ 255, 0, 0, 255 });

			unsigned int population = 0;

			for (unsigned x = 0; x < WorldLength; x++)
			{
				for (unsigned int y = 0; y < WorldLength; y++)
				{
					if (*(World + ((y * WorldLength) + x)) == AliveState)
					{
						DrawPixel(x, y, WHITE);
						population++;
					}
					else
					{
						DrawPixel(x, y, BLACK);
					}
				}
			}

			std::string generationtext = "Generation : ";
			std::string populationtext = "Population : ";
			std::string GPStext = "GPS : ";
			generationtext += std::to_string(Generation);
			populationtext += std::to_string(population);
			GPStext += std::to_string(GPS);

			DrawText(generationtext.c_str(), 0, WorldLength, 10, BLACK);
			DrawText(populationtext.c_str(), 0, WorldLength + 15, 10, BLACK);
			DrawText(GPStext.c_str(), 0, WorldLength + 30, 10, BLACK);

			EndDrawing();
		}

		RunLife = false;

		lifeloop.join();
	}

	CloseWindow();

	free(World);
	free(ConstantWorld);

	return 0;
}

void LifeThreadLoop()
{
	bool allowstep = false;

	while(RunLife)
	{	
		std::chrono::high_resolution_clock::time_point start = std::chrono::steady_clock::now();

		if (Paused == false)
		{
			LifeUpdate();
		}
		else
		{
			if (IsKeyDown(KEY_EQUAL))
			{
				if (allowstep == true)
				{
					allowstep = false;
					LifeUpdate();
				}
			}
			else
			{
				allowstep = true;
			}
		}

		std::chrono::high_resolution_clock::time_point end = std::chrono::steady_clock::now();

		std::chrono::nanoseconds microsecondcount = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

		double deltatime = microsecondcount.count() / 1000000000.0f;

		GPS = 1.0f / deltatime;
	}
}

inline void LifeUpdate()
{
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int aliveneighbourcount;
	int neighbourhoodx = 0;
	int neighbourhoody = 0;
	Cell* pointer = NULL;
	bool alive;

	memcpy(ConstantWorld, World, WorldLength * WorldLength * SizeOfCell);

	for (x = 0; x < WorldLength; x++)
	{
		for (y = 0; y < WorldLength; y++)
		{
			aliveneighbourcount = 0;

			for (neighbourhoodx = -NeighbourhoodSize; neighbourhoodx <= NeighbourhoodSize; neighbourhoodx++)
			{
				for (neighbourhoody = -NeighbourhoodSize; neighbourhoody <= NeighbourhoodSize; neighbourhoody++)
				{
					if (!(neighbourhoodx == 0 && neighbourhoody == 0))
					{
						pointer = ConstantWorld + ((((y + neighbourhoody) % WorldLength) * WorldLength) + ((x + neighbourhoodx) % WorldLength) * SizeOfCell);

						if (*pointer == AliveState)
						{
							aliveneighbourcount++;
						}
					}
				}
			}

			pointer = (World + (((y * WorldLength) + x) * SizeOfCell));
			alive = *pointer == AliveState;

			if (alive)
			{
				if (aliveneighbourcount < Underpopulation)
				{
					*pointer = DeadState;
				}

				if (aliveneighbourcount > Overpopulation)
				{
					*pointer = DeadState;
				}
			}
			else
			{
				if (aliveneighbourcount == Birth)
				{
					*pointer = AliveState;
				}
			}
		}
	}

	Generation++;
}