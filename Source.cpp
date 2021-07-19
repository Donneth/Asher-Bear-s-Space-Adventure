
#define OLC_PGEX_SOUND
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"

#include <algorithm>
#include <cstdint>
#include <array>


class Game : public olc::PixelGameEngine
{
public:
	
	Game()
	{
		sAppName = "Asher Bear's SPACE ADVENTURE v1.0";
	}

	olc::Sprite* spritePlayer = nullptr;
	olc::Sprite* spriteEnemy[3];
	olc::Sprite* spriteBoss = nullptr;
	olc::Sprite* kibble[1];
	olc::Sprite* kibbleS;
	olc::Sprite* spriteDrop[2];

	olc::vf2d vPlayerPos;
	float fPlayerSpeed = 100.0f;
	float fScrollSpeed = 45.0f;
	float fShipRad = 24 * 24;
	float fPlayerHealth = 1000.00f;
	float fGunTemp = 0.0f;
	float fGunReload = 0.2f;
	float fGunReloadTime = 0.0f;
	
	double dWorldPos = 0.0f;

	uint32_t GO_TOKEN = 0;
	uint32_t nLives = 3;
	uint32_t nShit = 4;
	int score;
	std::string printScore = std::to_string(score);
	std::string printLatestScore;

	bool retry = false;

	std::array<olc::vf2d, 1100>aryStars;

	olc::vf2d GetMiddle(olc::Sprite* s)
	{
		return { (float)s->width / 2.0f,(float)s->height / 2.0f };
	}


	
	struct sBullet
	{
		olc::vf2d Pos;
		olc::vf2d Vol;
		bool remove = false;
	};


	struct sEnemy;
	struct sBoss;

	struct sEnemyDefinition
	{
		double dTriggerTime;
		uint32_t nSpriteID = 0;
		float fHealth = 0.0f;
		std::function<void(sEnemy&, float, float)>funcMove;
		std::function<void(sEnemy&, float, float, std::list<sBullet>& bullets)>funcFire;
		float offset = 0.0f;
		bool bHasDrop = false;

	};
	
	struct sEnemy
	{
		olc::vf2d pos;
		sEnemyDefinition def;
		bool isDestroyed = false;

		std::array<float, 4>DataMove{ 0 };
		std::array<float, 4>DataFire{ 0 };


		void Update(float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			def.funcMove(*this, fElapsedTime, fScrollSpeed);
			def.funcFire(*this, fElapsedTime, fScrollSpeed, bullets);
		}

	};


	struct sBossDefinition : sEnemyDefinition
	{
		double dTriggerTime;
		uint32_t nSpriteID = 3;
		float fHealth = 0.0f;
		
		//Tracks Player pos
		std::function<void(sBoss&, olc::vf2d&, float, float)>funcTrack;	 
																		
		//Returns boss to start pos
		std::function<void(sBoss&, olc::vf2d&, float, float)>funcReturn; 

	};

	struct sBoss : sEnemy
	{
		olc::vf2d pos;
		olc::vf2d& vPlayerPos;
		sBossDefinition def;
		bool IsDestroyed = false;

		std::array<float, 4>DataMove{ 0 };
		std::array<float, 4>DataFire{ 0 };
		

		void Update(float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullet)
		{
			def.funcMove(*this, fElapsedTime, fScrollSpeed);
			def.funcFire(*this, fElapsedTime, fScrollSpeed, bullet);
			
			//Track
			def.funcTrack(*this, vPlayerPos, fElapsedTime, fScrollSpeed);
		}
	};
	
	struct sDrop;

	struct sDropDefinition
	{
		bool bCollected = false;
		uint32_t uPUP = 0;

		std::array<olc::Sprite, 2>dropArray;
		std::function<void(olc::Sprite&, float)>funcShow;
	};
	
	struct sDrop
	{
		olc::vf2d pos;
		sDropDefinition dropDef;
		bool bRemove = false;

		void Update(olc::vf2d& posDrop, olc::Sprite* sprite, uint32_t PUP_NUM, float fElapsedTime, float fScrollSpeed)
		{
		}
	};
	
	std::list<sEnemyDefinition>listSpawns;
	std::list<sEnemy>listEnemies;
	std::list<sDropDefinition>listDrop;
	std::list<sEnemyDefinition>listBoss;
	std::list<sBullet>listEnemyBullets;
	std::list<sBullet>listPlayerBullets;
	std::list<sBullet>listPlayerShit;
	std::list<olc::vf2d>listStars;
	std::list<sBullet>listFragments;
	
	
	bool OnUserCreate() override
	{

		spritePlayer = new olc::Sprite("gfx/BearPlayer.png");
		spriteEnemy[0] = new olc::Sprite("gfx/Pit.png");
		spriteEnemy[1] = new olc::Sprite("gfx/bEnemy1.png");
		spriteEnemy[2] = new olc::Sprite("gfx/bEnemy1.png");
		spriteBoss = new olc::Sprite("gfx/Pit_Boss.png");
		kibble[0] = new olc::Sprite("gfx/kibble.png");
		kibbleS = new olc::Sprite("gfx/kibble+.png");
		spriteDrop[0] = new olc::Sprite("gfx/upgrade_01.png");

		//Star Array Generation//
		for (auto& s : aryStars) s = { (float)(rand() % ScreenWidth()),
			(float)(rand() % ScreenHeight())};

		auto Drop_Upgrade = [&](sEnemy& dropper, sDrop& drop, olc::Sprite* sprite, float fElapsedTime, float fScrollSpeed)
		{
			if (dropper.def.bHasDrop && dropper.isDestroyed)
			{
				sprite = spriteDrop[drop.dropDef.uPUP];
				drop.pos += dropper.pos + olc::vf2d(sprite->width,sprite->height) * (fScrollSpeed / 5);
				drop.dropDef.bCollected = false;

				if (drop.dropDef.bCollected == true)
				{
					drop.bRemove = true;
				}
			}
		};

		//Movement Pattern Functions//
		auto Move_None = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.pos.y += fScrollSpeed * fElapsedTime;
		};


		auto Move_StraightFast = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.pos.y += 3.0f * (fScrollSpeed / 2.50) * fElapsedTime;
		};

		auto Move_StraightSlow = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.pos.y += 0.5f * fScrollSpeed * fElapsedTime;
		};

		auto Move_SinusoidNarrow = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.DataMove[0] += fElapsedTime;
			e.pos.y += 0.5f * fScrollSpeed * fElapsedTime;
			e.pos.x += 50.0f * cosf(e.DataMove[0]) * fElapsedTime;
		};

		auto Move_SinusoidWide = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.DataMove[0] += fElapsedTime;
			e.pos.y += 0.5f * fScrollSpeed * fElapsedTime;
			e.pos.x += 150.0f * cosf(e.DataMove[0]) * fElapsedTime;
		};

		auto Move_SinusoidWide_2 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{
			e.DataMove[0] += fElapsedTime * 1.5;
			e.pos.y += 0.2f * fScrollSpeed * fElapsedTime;
			e.pos.x += 450.0f * cosf(e.DataMove[0]) * fElapsedTime;

		};

	
		//FIRING PATTERN FUNCTIONS//

		auto Fire_None = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed)
		{

		};

		auto Fire_StraightDelay2 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			constexpr float fDelay = 0.2f;
			constexpr float nBullets = 10;
			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				sBullet b;
				b.Pos = e.pos + olc::vf2d((float)spriteEnemy[e.def.nSpriteID]->width / 2.0f, 
					(float)spriteEnemy[e.def.nSpriteID]->height / 2.0f);
				b.Vol = { 0.0f, 100.0f };
				bullets.push_back(b);
			}
		};

		auto Fire_Straight_Delay2_2 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullet)
		{
			constexpr float fDelay = 0.2f;
			constexpr int nBullets = 10;

			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				sBullet b;
				b.Pos = e.pos + olc::vf2d((float)spriteBoss->width / 2.0f,
					(float)spriteBoss->height / 2.0f);
				b.Vol = { 0.0f,100.0f };
				bullet.push_back(b);
			}
		};

		auto Fire_CirclePulse2 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			constexpr float fDelay = 0.9f;
			constexpr float nBullets = 10;
			constexpr float fTheta = 2.0f * 3.14159f / (float)nBullets;
			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				for (int i = 0; i < nBullets; i++)
				{
					sBullet b;
					b.Pos = e.pos + GetMiddle(spriteEnemy[e.def.nSpriteID]);
					b.Vol = { 100.0f * cosf(fTheta * i), 100.0f * sinf(fTheta * i) };
					bullets.push_back(b);
				}
			}
		};

		auto Fire_CirclePulse2_2 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullet)
		{
			constexpr float fDelay = 0.9f;
			constexpr int nBullets = 10;
			constexpr float fTheta = 2.0f * 3.141597 / (float)nBullets;

			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				for (int i = 0; i < nBullets; i++)
				{
					sBullet b;
					b.Pos = e.pos + GetMiddle(spriteBoss);
					b.Vol = { 100.0f * cosf(fTheta * i), 100.0f * sinf(fTheta * i) };

					bullet.push_back(b);
				}
			}
		};

		auto Fire_DeathSpiral = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			constexpr float fDelay = 0.8f;
			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[1] += 0.1f;
				e.DataFire[0] -= fDelay;
				sBullet b;
				b.Pos = e.pos + GetMiddle(spriteEnemy[e.def.nSpriteID]);
				b.Vol = { 100.0f * cosf(e.DataFire[1]), 100.0f * sinf(e.DataFire[1]) };
				bullets.push_back(b);
			}
		};

		auto Fire_DeathSpiralCircle = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			constexpr float fDelay = 1.0f;
			constexpr int nBullets = 10;
			constexpr float fTheta = 2.0f * 3.14159f / (float)nBullets;
			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				e.DataFire[1] += 0.1f;
				for (int i = 0; i < nBullets; i++)
				{
					sBullet b;
					b.Pos = e.pos + GetMiddle(spriteEnemy[e.def.nSpriteID]);
					b.Vol = { 100.0f * cosf(fTheta * i + e.DataFire[1]),100.0f 
						* sinf(fTheta * i + e.DataFire[1]) };
					bullets.push_back(b);
				}
			}
		};

		auto Boss_Fire_DeathCircle1 = [&](sEnemy& e, float fElapsedTime, float fScrollSpeed, std::list<sBullet>& bullets)
		{
			constexpr float fDelay = 0.2f;
			constexpr int nBullets = 10;
			constexpr float fTheta = 2 * 3.141579 / (float)nBullets;
			e.DataFire[0] += fElapsedTime;
			if (e.DataFire[0] >= fDelay)
			{
				e.DataFire[0] -= fDelay;
				e.DataFire[1] += 0.2f;

				for (int i = 0; i < nBullets; i++)
				{
					sBullet b;
					b.Pos = e.pos + GetMiddle(spriteBoss);
					b.Vol =
					{
						100.0f * cosf(fTheta * i + e.DataFire[1]), 100.0f *
						sinf(fTheta * i + e.DataFire[1])
					};
					bullets.push_back(b);
				}
			}
		};
		

			//Construct level//
		listSpawns =
		{
			{100.0, 0, 3.0f, Move_None,           Fire_StraightDelay2, 0.25f,true},
			{100.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.50f,true},
			{120.0, 1, 3.0f, Move_SinusoidNarrow, Fire_CirclePulse2, 0.75f,true},

			{200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.35f},
			{200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.70f},

			{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
			{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
			{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
			{500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

			{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.3f},
			{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.6f},
			{550.0, 0, 3.0f, Move_StraightSlow,   Fire_DeathSpiralCircle,  0.5f},
			{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.6f},
			{550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.9f},

			{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
			{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
			{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
			{600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

			{1100.0, 0, 3.0f, Move_None,           Fire_CirclePulse2, 0.25f},
			{1100.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.75f},
			{1120.0, 1, 3.0f, Move_SinusoidNarrow, Fire_CirclePulse2, 0.50f},

			{1200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.30f},
			{1200.0, 2, 3.0f, Move_SinusoidWide, Fire_CirclePulse2, 0.70f},

			{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
			{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
			{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
			{1500.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},

			{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.1f},
			{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.3f},
			{1550.0, 0, 3.0f, Move_StraightSlow,   Fire_DeathSpiralCircle,  0.5f},
			{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.7f},
			{1550.0, 0, 3.0f, Move_StraightFast,   Fire_DeathSpiral,  0.9f},

			{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.2f},
			{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.4f},
			{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.6f},
			{1600.0, 0, 3.0f, Move_StraightFast,   Fire_StraightDelay2,  0.8f},
			{1700.0, 3, 10.0f, Move_SinusoidWide_2, Boss_Fire_DeathCircle1, 0.15f},
		};
	
			vPlayerPos = { (float)ScreenWidth() / 2,(float)ScreenHeight() / 2 };

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//Draw score
		DrawString(37, 80, std::to_string(score));

		//AutoScroll world//
		dWorldPos += fScrollSpeed * fElapsedTime;

		//Scroll Player Object//
		vPlayerPos.y += fScrollSpeed * fElapsedTime;


		//Handke input//
		if (GetKey(olc::W).bHeld) vPlayerPos.y -= (fPlayerSpeed + fScrollSpeed) * fElapsedTime;
		if (GetKey(olc::S).bHeld) vPlayerPos.y += (fPlayerSpeed - fScrollSpeed) * fElapsedTime;
		if (GetKey(olc::A).bHeld) vPlayerPos.x -= fPlayerSpeed * fElapsedTime * 2.0f;
		if (GetKey(olc::D).bHeld) vPlayerPos.x += fPlayerSpeed * fElapsedTime * 2.0f;


		//"Clamps" player to screen//
		if (vPlayerPos.x <= 0) vPlayerPos.x = 0;
		if (vPlayerPos.x + (float)spritePlayer->width >= ScreenWidth()) vPlayerPos.x = (float)ScreenWidth() - spritePlayer->width;
		if (vPlayerPos.y <= 0) vPlayerPos.y = 0;
		if (vPlayerPos.y + (float)spritePlayer->height >= ScreenHeight()) vPlayerPos.y = (float)ScreenHeight() - spritePlayer->height;

		if (fPlayerHealth <= 0)
		{
			fPlayerSpeed = 0;
			fScrollSpeed = 0;

		}

		//Player Fire//
		bool bCanFire = false;
		fGunReloadTime -= fElapsedTime;
		if (fGunReloadTime <= 0.0f)
		{
			bCanFire = true;
		}


		fGunTemp -= fElapsedTime * 10.0f;
		if (fGunTemp < 0) fGunTemp = 0;
		if (GetMouse(0).bHeld && fPlayerHealth > 0.0f)
		{
			if (bCanFire && fGunTemp < 80.0f)
			{
				fGunReloadTime = fGunReload;
				fGunTemp += 5.0f;
				if (fGunTemp > 100.0f) fGunTemp = 100.0f;
				listPlayerBullets.push_back({
					vPlayerPos + olc::vf2d((float)spritePlayer->width / 2.0f,0.0f),{0.0f,-200.0f} });
			}
			if (fPlayerHealth <= 0.0f)
			{
				bCanFire = false;
				fGunReload = fElapsedTime * 10;
			}

		}

		if (GetMouse(1).bHeld && fPlayerHealth > 0.0f && nShit > 0)
		{
				listPlayerShit.push_back
				({
					vPlayerPos +
					olc::vf2d((float)spritePlayer->width / 4.72, 36.0f), {10.0f,200.0f}

					});
		}

		//Update Shit
		for (auto& s : listPlayerShit)
		{
			//Position Shit
			s.Pos += (s.Vol + olc::vf2d(0.0f, fScrollSpeed / 2)) * fElapsedTime;

			for (auto& e : listEnemies)
			{
				if ((s.Pos - (e.pos + olc::vf2d(12.0f, 12.0f))).mag2() < fShipRad)
				{
					e.def.fHealth -= 1.0f;
					s.remove = true;

					//"Trigger Death Explosion"//
					if (e.def.fHealth <= 0.0f)
					{
						for (int i = 0; i < 30; i++)
						{
							float angle = ((float)rand() / RAND_MAX * 2.0f * 3.1459f);
							float speed = ((float)rand() / RAND_MAX * 200.0f + 50.0f);
							listFragments.push_back({
								e.pos + GetMiddle(spriteEnemy[e.def.nSpriteID]),
								{ cosf(angle) * speed, sinf(angle) * speed } });
						}
					}
				}
			}
		}

		//Update Player Bullets//
		for (auto& b : listPlayerBullets)
		{
			//Position Bullets//
			b.Pos += (b.Vol + olc::vf2d(0.0f, fScrollSpeed)) * fElapsedTime;

			for (auto& e : listEnemies)
			{

				if ((b.Pos - (e.pos + olc::vf2d(12.0f, 12.0f))).mag2() < fShipRad)
				{
					//Enemy has been hit//
					e.def.fHealth -= 1.0f;
					b.remove = true;

					//"Trigger Death Explosion"//
					if (e.def.fHealth <= 0.0f)
					{
						for (int i = 0; i < 30; i++)
						{
							float angle = ((float)rand() / RAND_MAX * 2.0f * 3.1459f);
							float speed = ((float)rand() / RAND_MAX * 200.0f + 50.0f);
							listFragments.push_back({
								e.pos + GetMiddle(spriteEnemy[e.def.nSpriteID]),
								{ cosf(angle) * speed, sinf(angle) * speed } });
						}
					}
				}
			}
		}

		

		//Spawn Control//
		while (!listSpawns.empty() && dWorldPos >= listSpawns.front().dTriggerTime)
		{
			sEnemy e;

			e.def = listSpawns.front();
			e.pos =
			{
				listSpawns.front().offset * (float)(ScreenWidth() -
				spriteEnemy[e.def.nSpriteID]->width),
				0.0f - spriteEnemy[e.def.nSpriteID]->height
			};

			listSpawns.pop_front();
			listEnemies.push_back(e);

		}


		//Update Enemy//
		for (auto& e : listEnemies)
			e.Update(fElapsedTime, fScrollSpeed, listEnemyBullets);

		//Update Score//
		for (auto& e : listEnemies)
		{
			//Count score
			if (e.def.fHealth <= 0.0f)
				printScore = std::to_string(++score);
		}

		//Update Enemy Bullets//
		for (auto& b : listEnemyBullets)
		{
			//Position Bullet//
			b.Pos += (b.Vol + olc::vf2d(0.0f, fScrollSpeed)) * fElapsedTime;

			//Check Player Bullets Against Enemy Bullets//
			if ((b.Pos - (vPlayerPos + olc::vf2d(24.0f, 24.0f))).mag2() < fShipRad)
			{
				b.remove = true;
				fPlayerHealth -= 30.0f;
			}
		}

		//Update Fragments//
		for (auto& f : listFragments) f.Pos += (f.Vol + olc::vf2d(0.0f, fScrollSpeed)) * fElapsedTime * 2;

		//Remove Offscreen Enemies//
		listEnemies.remove_if([&](const sEnemy& e) {return (e.pos.y >= (float)ScreenHeight()) || e.def.fHealth <= 0.0f; });

		//Remove Finished Enemy Bullets//
		listEnemyBullets.remove_if([&](const sBullet& b) {return (b.Pos.x<0 || b.Pos.x > ScreenWidth() || b.Pos.y<0 || b.Pos.y>ScreenHeight() || b.remove); });

		//Remove Finished Fragments//
		listFragments.remove_if
		(
			[&](const sBullet& b)
			{return b.Pos.x < 0 || b.Pos.x > ScreenWidth() || b.Pos.y < 0 || b.Pos.y > ScreenHeight() || b.remove; }
		);

		//Remove player bullets//
		listPlayerBullets.remove_if
		(
			[&](const sBullet& b)
			{return b.remove; }
		);

		//Remove player shit//
		listPlayerShit.remove_if
		(
			[&](const sBullet& s)
			{return (s.remove || s.Pos.x < 0 || s.Pos.x > ScreenWidth() || s.Pos.y < 0 || s.Pos.y > ScreenHeight()); }
		);

		//GRAPHICS//
		Clear(olc::BLACK);

		if (fPlayerHealth <= 0.0f)
		{
			GO_TOKEN = 1;
			fPlayerHealth = 0.0f;
			fScrollSpeed = 0;
			fPlayerHealth = 0.0f;
			fElapsedTime += 0.0f;
			DrawString((float)ScreenHeight() / 2, ScreenHeight() / 2, "PRESS SPACE TO RETRY");

			if (fPlayerHealth <= 0.0f && nLives > 0)
			{
				if (GetKey(olc::SPACE).bHeld)
				{
					GO_TOKEN = 0;
					nLives--;
					retry = true;
					printLatestScore = printScore;
					score = 0;
					printScore = score;
					vPlayerPos = { (float)ScreenWidth() / 2, (float)ScreenHeight() / 2 };
					fPlayerHealth = 1000.0f;
					fPlayerSpeed = 100.0f;
					fScrollSpeed = 45.0f;
					fGunTemp = 0.0f;
					listEnemyBullets.clear();
					
				}		
			}

			if (nLives == 0)
				if (GetKey(olc::SPACE).bHeld)
					return OnUserDestroy();
		}
		
		

		//More star programming//
		for (size_t i = 0; i < aryStars.size(); i++)
		{
			auto& s = aryStars[i];
			s.y += ((fScrollSpeed * 2) * fElapsedTime) * ((i < aryStars.size() >> 1) ? 0.0f : 1.0f);
			if (s.y > (float)ScreenHeight())
				s.y = { (float)(rand() % ScreenHeight()) };
			s.x += ((fScrollSpeed * 2) * fElapsedTime) * ((i < aryStars.size() >> 1) ? 0.0f : 1.0f);
			if (s.x > (float)ScreenWidth())
				s.x = { (float)(rand() % ScreenWidth()) };

			Draw(s, (i < aryStars.size() >> 1) ? olc::DARK_GREY : olc::WHITE);
		}
		SetPixelMode(olc::Pixel::MASK);

		//Draw Enemies//
		for (auto& e : listEnemies)
		{
			DrawSprite(e.pos, spriteEnemy[e.def.nSpriteID]);
		}

		//Draw Player//
		DrawSprite(vPlayerPos, spritePlayer);
		SetPixelMode(olc::Pixel::NORMAL);

		//Draw Enemy Bullets//
		for (auto& b : listEnemyBullets) FillCircle(b.Pos, 3, olc::RED);

		//Draw Player Bullets//
		for (auto& b : listPlayerBullets)
		{
			DrawSprite(b.Pos, kibble[0]);
		}

		//Draw Fragments//
		for (auto& f : listFragments)
		{
			DrawSprite(f.Pos, kibbleS);
		}

		//Draw Shit//
		for (auto& s : listPlayerShit)
		{
			DrawSprite(s.Pos, kibbleS);
		}
		

		//Draw Player Health Bar//
		DrawString(4, 4, "HEALTH: ");
		FillRect(60, 4, (fPlayerHealth / 1000.0f * 576.0f), 8, olc::GREEN);


		//WEAPON//
		DrawString(12, 16, "WEAPON: ");
		DrawString(32, 30, std::to_string((double)fGunTemp));
		FillRect(68, 16, (fGunTemp / 100.0f * 576.0f), 8, olc::YELLOW);

		//Draw Score
		DrawString(27, 60, "SCORE: ", olc::YELLOW);
		DrawString(37, 80, std::to_string(score));
		DrawString(27, 100, "LATEST: ");
		DrawString(37, 120, printLatestScore);
	
		//Draw Lives
		DrawString(37, 160, std::to_string(nLives));
		DrawString(27, 140, "LIVES: ");

		//Instructions//
		if ((fGunTemp == 0.0f) && (GO_TOKEN != 1))
			DrawString(120, 360, "CLICK TO SHOOT");
		else
			DrawString(120, 360, " ");

		if (!(GetKey(olc::W).bHeld ||
			GetKey(olc::A).bHeld ||
			GetKey(olc::S).bHeld ||
			GetKey(olc::D).bHeld) && (fGunTemp == 0.0f) && (GO_TOKEN != 1))
			DrawString(ScreenWidth() / 2, ScreenHeight() / 2.5, "WASD to move");
		
		//Draw ElapsedTime
		/*
		DrawString(ScreenWidth() / 3, ScreenHeight() / 16, 
			std::to_string(std::time(strftime())));
		*/

		return true;
	}

	bool OnUserDestroy()
	{
		exit(0);

		return true;
	}

	void ResetScore()
	{
		if (this->score != 0)
			this->score = 0;
		DrawString(37, 80, std::to_string(0));
	}
};


int main()
{
	Game g;
	
	if (g.Construct(640, 480, 2, 2))
	{
		g.Start();
	}


	
	return 0;
}