#pragma once
#include <iostream>
#include <list>
#include <map>
#include <random>
#include "macros.h"

using namespace std;

class ExchangeItem : public less<string> {
	typedef struct shareItem {
		int shares;
		int valid;
		string orderId;
	}ShareItem;
private:
	string price;
	OrderEnum orderType;
	string orderId;
	ExchangeEnum exchangeType;
	int shares;
	/**
	 * Used in Exchange Center
	 */
	list<ShareItem *> sharesQueue;
	typedef list<ShareItem*>::iterator SharesIterator;
	map<string, ShareItem*> sharesMap;

	/**
	 * Used in Trader
	 */
	// list<ShareItem*> traderQueue;

public:
	ExchangeItem() {}

	ExchangeItem(string price, OrderEnum orderType, string orderId, ExchangeEnum exchangeType, int shares) {
		this->price = price;
		this->orderType = orderType;
		this->orderId = orderId;
		this->exchangeType = exchangeType;
		this->shares = shares;
	}
	~ExchangeItem() {
		clean();
	}

	ExchangeItem& operator=(const ExchangeItem& right) {
		this->price = right.price;
		this->orderType = right.orderType;
		this->orderId = right.orderId;
		this->exchangeType = right.exchangeType;
		this->shares = right.shares;
		this->sharesQueue = right.sharesQueue;
		this->sharesMap = right.sharesMap;

		return *this;
	}

	static string get(const ExchangeItem& self) {
		return self.price;
	}

#define delimeter ","
	bool parseLine(char * str, int len) {
		// only parse fixed length
		const int paramSize = 5;
		char* pTmp = NULL;
		char * token = strtok_s(str, delimeter, &pTmp);

		int i = 0;
		while (token != NULL && i < paramSize) {
			// TODO: verify read token
			if (i == 0) {
				// read OrderType 
				this->orderType = ConvertStringToEnum<OrderEnum>(token);
			}

			if (i == 1) {
				// read orderId
				this->orderId = token;
			}

			if (i == 2) {
				// read ExchangeType
				this->exchangeType = ConvertStringToEnum<ExchangeEnum>(token);
			}

			if (i == 3) {
				// read shares
				this->shares = stoi(token);
			}

			if (i == 4) {
				// read price
				this->price = token;
			}

			i++;
			token = strtok_s(NULL, delimeter, &pTmp);
		}

		//this->print();

		return true;
	}

	void print(){
		cout << GET_ENUM_STR(OrderEnum, this->orderType) << ":"
			 << this->orderId << ":"
			 << GET_ENUM_STR(ExchangeEnum, this->exchangeType) << ":"
			 << this->shares << ":"
		     << this->price
			 << endl;
	}

	void print(ExchangeItem * eItem, ShareItem * sItem) {
		cout << GET_ENUM_STR(OrderEnum, eItem->orderType) << ":"
			<< sItem->orderId << ":"
			<< GET_ENUM_STR(ExchangeEnum, eItem->exchangeType) << ":"
			<< sItem->shares << ":"
			<< eItem->price
			<< endl;
	}

	void printAll(bool ask = false) {
		for (auto shareItem : sharesQueue) {
			if (!shareItem->valid) {
				continue;
			}

			if (ask && this->exchangeType == ExchangeEnum::S) {
				cout << GET_ENUM_STR(OrderEnum, this->orderType) << ":"
					<< shareItem->orderId << ":"
					<< GET_ENUM_STR(ExchangeEnum, this->exchangeType) << ":"
					<< shareItem->shares << ":"
					<< this->price
					<< endl;
				continue;
			}

			if (!ask && this->exchangeType == ExchangeEnum::B) {
				cout << GET_ENUM_STR(OrderEnum, this->orderType) << ":"
					<< shareItem->orderId << ":"
					<< GET_ENUM_STR(ExchangeEnum, this->exchangeType) << ":"
					<< shareItem->shares << ":"
					<< this->price
					<< endl;
			}
		}
	}

	bool empty() {
		return sharesQueue.empty();
	}

	void pushBack(int shares, string orderId) {
		ShareItem* sItem = new ShareItem();
		sItem->shares = shares;
		sItem->orderId = orderId;
		sItem->valid = true;
		sharesQueue.push_back(sItem);

		sharesMap.insert(pair<string, ShareItem*>(sItem->orderId, sItem));
	}

	ShareItem * popFront() {
		if (sharesQueue.empty()) {
			return 0;
		}

		ShareItem * item = sharesQueue.front();
		sharesQueue.pop_front();
		if (item->valid) {
			// lazy delete sharesQueue affected, avoid memory leak
			sharesMap.erase(item->orderId);
		}
		return item;
	}
	void pushFront(int shares, string orderId) {
		ShareItem* newItem = new ShareItem();
		newItem->orderId = orderId;
		newItem->shares = shares;
		newItem->valid = true;
		sharesQueue.push_front(newItem);
		sharesMap.insert(pair<string, ShareItem*>(newItem->orderId, newItem));
	}
	void pushFront(ShareItem* newItem) {
		sharesQueue.push_front(newItem);
	}
	void clean() {
		ShareItem* curItem = NULL;
		while (curItem = popFront()) {
			sharesMap.erase(curItem->orderId);
			delete curItem;
			curItem = NULL;
		}
	}

	void cancel(ExchangeItem * cancelItem) {
		// get match order
		// TODO: implement LinkedHashMap
		map<string, ShareItem *>::iterator it = sharesMap.find(cancelItem->orderId);
		if (it == sharesMap.end() || (it->second)->shares != cancelItem->getShares()) {
			// wrong shares, skip
			cout << "Mismatch order shares: " << endl;
			cout << "    Old: ";
			this->print();
			cout << "    Current: ";
			cancelItem->print();

			delete cancelItem;
			cancelItem = NULL;
			return;
		}

		cancelItem->print();

		// erase item in sharesQueue & sharesMap
		ShareItem * curItem = (it->second);
		curItem->valid = false;
		sharesMap.erase(it);
	}

	bool merge(ExchangeItem * newItem) {
		// skip cancelled shareItem, lock() in multi-thread env
		ShareItem* sItem = NULL;
		int leftQuantity = newItem->getShares();
		while (sItem = this->popFront()) {
			if (!sItem->valid) {
				delete sItem;
				sItem = NULL;
				continue;
			}
			// match, subtract, if not enough, add
			// no matter current item is 'S' or 'B'
			int toDealQuantity = sItem->shares;

			// if match exactly
			if (leftQuantity == toDealQuantity) {
				leftQuantity = 0;
				cout << "Executed: ";
				print(this, sItem);
				break;
			}
			if (leftQuantity < toDealQuantity) {
				// subtract from current item
				sItem->shares -= leftQuantity;
				leftQuantity = 0;
				pushFront(sItem);
				cout << "Executed: ";
				print(this, sItem);
				break;
			}
			// not enough shares to deal, subtract & next()
			leftQuantity -= toDealQuantity;
			cout << "Executed: ";
			print(this, sItem);
		}

		// still space, update un-executed shares
		if (leftQuantity > 0) {
			newItem->setShares(leftQuantity);
			return false;
		}

		return true;
	}

	const string getPrice() {
		return price;
	}
	OrderEnum getOrderType() {
		return orderType;
	}
	void setOrderType(OrderEnum orderType) {
		this->orderType = orderType;
	}
	string getOrderId() {
		return orderId;
	}
	void setOrderId() {
		this->orderId = getRandomStr();
	}
	string getRandomStr() {
		string str;
		for (int i = 0; i < 10; i++) {
			str += to_string(rand() % 9);
		}
		return str;
	}

	ExchangeEnum getExchangeType() {
		return exchangeType;
	}
	int getShares() {
		return shares;
	}
	void setShares(int shares) {
		this->shares = shares;
	}
};
