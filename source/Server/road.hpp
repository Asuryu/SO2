#pragma once

#include "entity/frog.hpp"
#include "entity/entity.hpp"
#include "entity/mentity.hpp"

#include <vector>

import settings;

class Road {
public:
	Road(int size) {
		this->size = size;
	}

	bool processTick()
	{
		bool ret = false;
		for (auto& entity : entities)
			ret |= entity->processTick();
		
		return ret;
	}

	void invert() {
		for (auto& entity : entities)
			entity->invertFacingDirection();
	}

	void setFrozen(bool frozen) {
		for (auto& entity : entities) {
			auto mentity = dynamic_cast<MovingEntity*>(entity);
			if (mentity == nullptr)
				continue;

			mentity->setMoving(!frozen);
		}
	}

	void addEntity(Entity* entity) {
		entities.push_back(entity);
	}

	void removeEntity(Entity* entity) {
		entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
	}

	std::vector<Entity*> getEntities() {
		return entities;
	}
private:
	int size;

	std::vector<Entity*> entities;
};