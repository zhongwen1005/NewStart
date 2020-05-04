#pragma once
#include "ExchangeItem.hpp"
#include "FileHandler.hpp"
#include <queue>

using namespace std;

class AlgoTrader {
private:
	list<ExchangeItem *> oldSets;
	list<ExchangeItem *> newSets;
	map<string, queue<ExchangeItem*>*> oldMap;
    map<string, queue<ExchangeItem*>*> newMap;
    typedef list<ExchangeItem*>::iterator ListIter;
	typedef map<string, queue<ExchangeItem*>*>::iterator MapIter;

public:
	AlgoTrader() {}
	~AlgoTrader() {
        for (ListIter listIter = oldSets.begin(); listIter != oldSets.end(); listIter++) {
            delete (*listIter);
        }
        oldSets.clear();
        oldMap.clear();

        for (ListIter listIter = newSets.begin(); listIter != newSets.end(); listIter++) {
            delete (*listIter);
        }
        newSets.clear();
        newMap.clear();
    }

	void LoadOldSet() {
		FileHandler<ExchangeItem> fileHandler;
		fileHandler.open("traderOld.txt");
		fileHandler.skipFirstLine();

		ExchangeItem* currentLine = nullptr;
        cout << "Old Set: " << endl;
		while (currentLine = fileHandler.readLine()) {
            MapIter mapIter = oldMap.find(currentLine->getPrice());
            if (mapIter != oldMap.end()) {
                // match, append
                mapIter->second->push(currentLine);
                currentLine->print();
                continue;
            }
            oldSets.push_back(currentLine);
            auto oldQueue = new queue<ExchangeItem*>();
            oldQueue->push(currentLine);
            oldMap.insert(pair<string, queue<ExchangeItem*> *>(currentLine->getPrice(), oldQueue));
            currentLine->print();
		}
	}

	void LoadNewSet() {
		FileHandler<ExchangeItem> fileHandler;
		fileHandler.open("traderNew.txt");
		fileHandler.skipFirstLine();

		ExchangeItem* currentLine = nullptr;
        cout << "New Set: " << endl;
		while (currentLine = fileHandler.readLine()) {
            MapIter mapIter = newMap.find(currentLine->getPrice());
            if (mapIter != newMap.end()) {
                // match, append
                mapIter->second->push(currentLine);
                currentLine->print();
                continue;
            }
            newSets.push_back(currentLine);
            auto newQueue = new queue<ExchangeItem*>();
            newQueue->push(currentLine);
            newMap.insert(pair<string, queue<ExchangeItem*>*>(currentLine->getPrice(), newQueue));
			currentLine->print();
		}
	}

	void intersect() {
		/**
		 * Assume input sets are ordered, 
		 */
        // init two cursor, act as concurrent movement
        ListIter oldSetsIter = oldSets.begin();
        ListIter newSetsIter = newSets.begin();
        MapIter oldItemIter = oldMap.begin();
        MapIter newItemIter = newMap.begin();
        // inputSet is based on initialSet, so there should be existing orders
        for (; oldSetsIter != oldSets.end(); oldSetsIter++) {
            // Process inputSet first, prevent the outer loop finished
            // get the first item in InputSet
            // assume match the outstanding orders
            while (newSetsIter != newSets.end()) {
                oldItemIter = oldMap.find((*newSetsIter)->getPrice());
                if (oldItemIter == oldMap.end()) {
                    // not exist, new order
                    newItemIter = newMap.find((*newSetsIter)->getPrice());
                    NewOrder(newItemIter);
                    newSetsIter++;
                    continue;
                }
                // key exists, merge with oldSets
                break;
            }

            // newSetsIter should be ++ before process next oldSetsIter

            // check if old record exists in new sets
            oldItemIter = newMap.find((*oldSetsIter)->getPrice());
			if (oldItemIter == newMap.end()) {
				// not exist, cancelled order
				oldItemIter = oldMap.find((*oldSetsIter)->getPrice());
				CancelOrder(oldItemIter);
				// newsSetsIter should move concurrently with oldSetsIter
				newSetsIter++;
				continue;
			}

            MergeQueue(newItemIter, oldItemIter, (*oldSetsIter)->getPrice());

            // newsSetsIter should move concurrently with oldSetsIter
            newSetsIter++;
        }

        for (; newSetsIter != newSets.end(); newSetsIter++) {
            // new add
            MapIter mapIter = newMap.find((*newSetsIter)->getPrice());
            if (mapIter != oldMap.end()) {
                while (! mapIter->second->empty()) {
                    ExchangeItem* newItem = (mapIter->second)->front();
                    newItem->setOrderId();
                    newItem->print();
                    mapIter->second->pop();
                }
                continue;
            }
        }
	}

    void MergeQueue(AlgoTrader::MapIter& newItemIter, AlgoTrader::MapIter& oldItemIter, string price)
    {

        // get oldQueue & newQueue
        newItemIter = newMap.find(price);
        oldItemIter = oldMap.find(price);
        // both cursor point to the same Price
        // compare two queues, i think the best option is to modify, not addup then cancel
        ExchangeItem* oldItem = NULL;
        ExchangeItem* newItem = NULL;

        // newItemIter(queue), oldItemIter(queue)            
        while (true) {
            newItem = NULL;
            if (!newItemIter->second->empty()) {
                newItem = newItemIter->second->front();
                newItemIter->second->pop();
            }
            oldItem = NULL;
            if (!oldItemIter->second->empty()) {
                oldItem = oldItemIter->second->front();
                oldItemIter->second->pop();
            }

            // process remaining items outside
            if (newItem == NULL || oldItem == NULL) {
                break;
            }

            if (oldItem->getShares() == newItem->getShares()) {
                // no changes
                continue;
            }
            // modify oldItem
            oldItem->setOrderType(OrderEnum::M);
            oldItem->setShares(newItem->getShares());
            oldItem->print();
        }

        // process remaining items
        if (newItem == NULL) {
            // prcess old queue, cancel
            while (oldItem) {
                // Cancel oldItem
                oldItem->setOrderType(OrderEnum::X);
                oldItem->print();

                oldItem = NULL;
                if (!oldItemIter->second->empty()) {
                    oldItem = oldItemIter->second->front();
                    oldItemIter->second->pop();
                }
            }
        }

        if (oldItem == NULL) {
            // process new queue, add
            while (newItem) {
                // modify oldItem
                newItem->setOrderType(OrderEnum::A);
                newItem->setOrderId();
                newItem->print();

                newItem = NULL;
                if (!newItemIter->second->empty()) {
                    newItem = newItemIter->second->front();
                    newItemIter->second->pop();
                }
            }
        }
    }

    void CancelOrder(AlgoTrader::MapIter& oldItemIter)
    {
        while (!oldItemIter->second->empty()) {
            ExchangeItem* oldItem = (oldItemIter->second)->front();
            oldItem->setOrderType(OrderEnum::X);
            oldItem->print();
            oldItemIter->second->pop();
        }
    }

    void NewOrder(AlgoTrader::MapIter& mapIter)
    {
        while (!mapIter->second->empty()) {
            ExchangeItem* newItem = (mapIter->second)->front();
            newItem->setOrderType(OrderEnum::A);
            newItem->setOrderId();
            newItem->print();
            mapIter->second->pop();
        }
    }
};
