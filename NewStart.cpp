#include "macros.h"
#include "FileHandler.hpp"
#include "btree.hpp"
#include "ExchangeItem.hpp"
#include "AlgoTrader.hpp"

typedef tlx::BTree<string, ExchangeItem, ExchangeItem, ExchangeItem> BTree;
typedef tlx::BTree<string, ExchangeItem, ExchangeItem, ExchangeItem>::iterator BTreeIter;

bool match(BTree* bTree, const string& key);
void trader();
void exchange();

int main(int argc, char ** argv)
{
    if (argc >= 2 && strcmp(argv[1], "-t") == 0) {
        trader();
        return 0;
    }

    exchange();
    return 0;
}

void trader() {
    /**
     * Step 1: load old set
     */
    AlgoTrader trader;
    trader.LoadOldSet();
    cout << endl;
    /**
     * Step 1: load old set
     */
    trader.LoadNewSet();
    cout << endl;
    
    cout << "Results Set: " << endl;
    trader.intersect();
}

void exchange() {
    /**
     * Step 1: load exchange.txt
     *   Format: A,100000,S,1,1075
     */
    FileHandler<ExchangeItem> fileHandler;
    fileHandler.open("exchange.txt");
    fileHandler.skipFirstLine();

    /**
     * Step 2: iterate each line
     *     if can be executed, merge
     */
     // TODO: ignore order ID for the time being
    auto bTree = new BTree();
    ExchangeItem* currentLine = nullptr;
    while (currentLine = fileHandler.readLine()) {
        // TODO: Modify should trigger merge process

        // check if exists or not
        BTreeIter iterator = bTree->find(currentLine->getPrice());
        if (iterator == bTree->end()) {
            if (currentLine->getOrderType() != OrderEnum::A) {
                // skip unmatch price, non A order
                cout << "Skip invalid record: ";
                currentLine->print();
                continue;
            }
            // insert
            currentLine->pushBack(currentLine->getShares(), currentLine->getOrderId());
            bTree->insert(*currentLine);
            currentLine->print();
            continue;
        }

        if (currentLine->getOrderType() == OrderEnum::X) {
            // cancel order
            iterator->cancel(currentLine);
            continue;
        }

        if (currentLine->getExchangeType() == iterator->getExchangeType()) {
            // append new order
            iterator->pushBack(currentLine->getShares(), currentLine->getOrderId());
            currentLine->print();
            // save memory
            delete currentLine;
            currentLine = NULL;
            continue;
        }

        // exist, do merge, skip & pop invalid item
        // TODO: aggressive asking or biding
        // In shares List, after merge, only one type of shares left   
        cout << "Processing: ";
        currentLine->print();
        if (!iterator->merge(currentLine)) {
            // all items were processed, replace current node
            bTree->erase(iterator);
            currentLine->pushBack(currentLine->getShares(), currentLine->getOrderId());
            bTree->insert(*currentLine);
            cout << "Left: ";
            currentLine->print();
        }
        if (iterator->empty()) {
            // remove current node
            bTree->erase(iterator);
        }
    }

    // print order book
    cout << endl << endl;
    cout << "ASK: " << endl;
    for (auto item = bTree->begin(); item != bTree->end(); item++) {
        item->printAll(true);
    }
    // print order book
    cout << endl;
    cout << "BID: " << endl;
    for (auto item = bTree->begin(); item != bTree->end(); item++) {
        item->printAll();
    }

    delete bTree;
}

void test() {
    auto bTree = new BTree();
    std::string insertKey1("133");
    ExchangeItem* eItem = new ExchangeItem(insertKey1, OrderEnum::A, "10001", ExchangeEnum::B, 100);
    bTree->insert(*eItem);

    std::string insertKey2("124");
    ExchangeItem* eItem2 = new ExchangeItem(insertKey2, OrderEnum::A, "10001", ExchangeEnum::B, 100);
    bTree->insert(*eItem2);

    std::string insertKey3("125");
    ExchangeItem* eItem3 = new ExchangeItem(insertKey3, OrderEnum::A, "10001", ExchangeEnum::B, 100);
    bTree->insert(*eItem3);

    match(bTree, "124");
    match(bTree, "125");
    match(bTree, "123");
    match(bTree, "133");
}

bool match(BTree* bTree, const string & key) {
    BTreeIter iterator = bTree->find(key);

    if (iterator != bTree->end()) {
        std::cout << "Match: " << iterator->getPrice() << endl;
        return true;
    }

    std::cout << "Key( " << key << ") No match!" << endl;

    return false;
}