#include <cxxtest/TestSuite.h>
#include "transfer/DiskCache.hpp"
#include "transfer/MemoryCache.hpp"
#include "transfer/NetworkTransfer.hpp"
#include "transfer/TransferManager.hpp"
#include "transfer/TransferData.hpp"
#include "transfer/LRUPolicy.hpp"

#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace Sirikata;

/*  Created on: Jan 10, 2009 */

#define EXAMPLE_HASH "55ca2e1659205d752e4285ce927dcda19b039ca793011610aaee3e5ab250ff80"

class TransferTestSuite : public CxxTest::TestSuite
{
	typedef Transfer::CacheLayer CacheLayer;
	std::vector< CacheLayer*> mCacheLayers;
	volatile int finishedTest;

	boost::mutex wakeMutex;
	boost::condition_variable wakeCV;
public:
	TransferTestSuite () {
	}

	virtual void setUp() {
		finishedTest = 0;
		mCacheLayers.clear();
	}

	CacheLayer *createTransferLayer(CacheLayer *next=NULL) {
		CacheLayer *layer = new Transfer::NetworkTransfer(next);
		mCacheLayers.push_back(layer);
		return layer;
	}
	CacheLayer *createDiskCache(CacheLayer *next = NULL,
				int size=32000,
				std::string dir="diskCache") {
		CacheLayer *layer = new Transfer::DiskCache(
							new Transfer::LRUPolicy(size),
							dir,
							next);
		mCacheLayers.push_back(layer);
		return layer;
	}
	CacheLayer *createMemoryCache(CacheLayer *next = NULL,
				int size=3200) {
		CacheLayer *layer = new Transfer::MemoryCache(
							new Transfer::LRUPolicy(size),
							next);
		mCacheLayers.push_back(layer);
		return layer;
	}
	CacheLayer *createSimpleCache(bool memory, bool disk, bool http) {
		CacheLayer *firstCache = NULL;
		if (http) {
			firstCache = createTransferLayer(firstCache);
		}
		if (disk) {
			firstCache = createDiskCache(firstCache);
		}
		if (memory) {
			firstCache = createMemoryCache(firstCache);
		}
		return firstCache;
	}
	void tearDownCache() {
		for (std::vector<Transfer::CacheLayer*>::reverse_iterator iter =
					 mCacheLayers.rbegin(); iter != mCacheLayers.rend(); ++iter) {
			delete (*iter);
		}
		mCacheLayers.clear();
	}

	virtual void tearDown() {
		tearDownCache();
		finishedTest = 0;
	}

	static TransferTestSuite * createSuite( void ) {
		return new TransferTestSuite();
	}
	static void destroySuite(TransferTestSuite * k) {
		delete k;
	}

	void waitFor(int numTests) {
		boost::unique_lock<boost::mutex> wakeup(wakeMutex);
		while (finishedTest < numTests) {
			wakeCV.wait(wakeup);
		}
	}
	void notifyOne() {
		boost::unique_lock<boost::mutex> wakeup(wakeMutex);
		finishedTest++;
		wakeCV.notify_one();
	}

	void callbackExampleCom(const Transfer::URI &uri, const Transfer::SparseData *myData) {
		TS_ASSERT(myData != NULL);
		if (myData) {
			myData->debugPrint(std::cout);
			const Transfer::DenseData &densedata = (*myData->begin());
			TS_ASSERT_EQUALS (SHA256::computeDigest(densedata.data(), densedata.length()), uri.fingerprint());
		} else {
			std::cout << "fail!" << std::endl;
		}
		std::cout << "Finished displaying!" << std::endl;
		notifyOne();
		std::cout << "Finished callback" << std::endl;
	}

	bool doExampleComTest( CacheLayer *transfer ) {
		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");

		bool async = transfer->getData(exampleComUri,
				Transfer::Range(true),
				boost::bind(&TransferTestSuite::callbackExampleCom, this, exampleComUri, _1));

		waitFor(1);

		std::cout << "Finished example.com test" << std::endl;
		return async;
	}

	void testDiskCache_exampleCom( void ) {
		CacheLayer *testCache = createSimpleCache(true, true, true);
		testCache->purgeFromCache(SHA256::convertFromHex(EXAMPLE_HASH));
		doExampleComTest(testCache);
		tearDown();
		// Ensure that it is now in the disk cache.
		doExampleComTest(createSimpleCache(false, true, false));
	}
	void testMemoryCache_exampleCom( void ) {
		CacheLayer *disk = createDiskCache();
		CacheLayer *memory = createMemoryCache(disk);
		// test disk cache.
		std::cout << "Testing disk cache..."<<std::endl;
		doExampleComTest(memory);

		// test memory cache.
		memory->setNext(NULL);
		// ensure it is not using the disk cache.
		std::cout << "Testing memory cache..."<<std::endl;
		doExampleComTest(memory);
	}

	void simpleCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			//myData->debugPrint(std::cout);
		}
		notifyOne();
	}

	void checkNullCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData==NULL);
		notifyOne();
	}

	void checkOneDenseDataCallback(const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			Transfer::SparseData::const_iterator iter = myData->begin();
			TS_ASSERT(++iter == myData->end());
		}
		notifyOne();
	}

	void testCleanup( void ) {
		Transfer::URI testUri (SHA256::computeDigest("01234"), "http://www.google.com/");
		Transfer::URI testUri2 (SHA256::computeDigest("56789"), "http://www.google.com/intl/en_ALL/images/logo.gif");
		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");

		Transfer::TransferCallback simpleCB = boost::bind(&TransferTestSuite::simpleCallback, this, _1);
		Transfer::TransferCallback checkNullCB = boost::bind(&TransferTestSuite::checkNullCallback, this, _1);

		CacheLayer *transfer = createSimpleCache(true, true, true);

		transfer->purgeFromCache(testUri.fingerprint());
		transfer->purgeFromCache(testUri2.fingerprint());
		transfer->getData(testUri, Transfer::Range(true), checkNullCB);
		transfer->getData(testUri2, Transfer::Range(true), checkNullCB);

		// example.com should be in disk cache--make sure it waits for the request.
		// disk cache is required to finish all pending requests before cleaning up.
		transfer->getData(exampleComUri, Transfer::Range(true), simpleCB);

		// do not wait--we want to clean up these requests.
	}

	void testOverlappingRange( void ) {
		Transfer::TransferCallback simpleCB = boost::bind(&TransferTestSuite::simpleCallback, this, _1);
		int numtests = 0;

		CacheLayer *http = createTransferLayer();
		CacheLayer *disk = createDiskCache(http);
		CacheLayer *memory = createMemoryCache(disk);

		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");
		memory->purgeFromCache(exampleComUri.fingerprint());

		std::string diskFile = "diskCache/" + exampleComUri.fingerprint().convertToHexString() + ".part";
		// First test: GetData something which will overlap with the next two.
		printf("1\n");
		http->getData(exampleComUri,
				Transfer::Range(6, 10, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		// Now getData two pieces (both of these should kick out the first one)
		printf("2/3\n");
		http->getData(exampleComUri,
				Transfer::Range(2, 8, Transfer::BOUNDS),
				simpleCB);
		http->getData(exampleComUri,
				Transfer::Range(8, 14, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=2);

		// Now check that an overlapping range from before doesn't cause problems
		printf("4\n");
		http->getData(exampleComUri,
				Transfer::Range(6, 13, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("5 -> THIS ONE IS FAILING\n");
		// Everything here should be cached
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(5, 8, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("6 -> THIS ONE IS FAILING?\n");
		// And the whole range we just got.
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, 14, Transfer::BOUNDS),
				simpleCB);

		waitFor(numtests+=1);

		printf("7\n");
		// getDatas from 2 to the end. -- should not be cached
		// and should overwrite all previous ranges because it is bigger.
		memory->setNext(disk);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				simpleCB);
		waitFor(numtests+=1);

		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				boost::bind(&TransferTestSuite::checkOneDenseDataCallback, this, _1));
		waitFor(numtests+=1);

		// Whole file trumps anything else.
		memory->setNext(disk);
		memory->getData(exampleComUri,
				Transfer::Range(true),
				simpleCB);
		waitFor(numtests+=1);

		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, true),
				boost::bind(&TransferTestSuite::checkOneDenseDataCallback, this, _1));
		waitFor(numtests+=1);

		// should be cached
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(2, 14, Transfer::BOUNDS),
				simpleCB);
		waitFor(numtests+=1);

		// should be 1--end should be cached as well.
		memory->setNext(NULL);
		memory->getData(exampleComUri,
				Transfer::Range(1, 10, Transfer::BOUNDS, true),
				simpleCB);
		waitFor(numtests+=1);
	}

	void compareCallback(Transfer::DenseDataPtr compare, const Transfer::SparseData *myData) {
		TS_ASSERT(myData!=NULL);
		if (myData) {
			Transfer::Range::base_type offset = compare->startbyte();
			while (offset < compare->endbyte()) {
				Transfer::Range::length_type len;
				const unsigned char *gotData = myData->dataAt(offset, len);
				const unsigned char *compareData = compare->dataAt(offset);
				TS_ASSERT(gotData);
				if (!gotData) {
					break;
				}
				bool equal = memcmp(compareData, gotData, len+offset<compare->endbyte() ? len : compare->endbyte()-offset)==0;
				TS_ASSERT(equal);
				if (!equal) {
					std::cout << std::endl << "WANT =======" << std::endl;
					std::cout << compareData;
					std::cout << std::endl << "GOT: =======" << std::endl;
					std::cout << gotData;
					std::cout << std::endl << "-----------" << std::endl;
				}
				offset += len;
				if (offset >= compare->endbyte()) {
					break;
				}
			}
		}
		notifyOne();
	}

	void testRange( void ) {
		Transfer::URI exampleComUri (SHA256::convertFromHex(EXAMPLE_HASH), "http://example.com/");
		CacheLayer *http = createTransferLayer();
		CacheLayer *disk = createDiskCache(http);
		CacheLayer *memory = createMemoryCache(disk);

		memory->purgeFromCache(exampleComUri.fingerprint());
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(2, 6, Transfer::LENGTH)));
			memcpy(expect->writableData(), "TML>\r\n", expect->length());
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					boost::bind(&TransferTestSuite::compareCallback, this, expect, _1));
		}
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(8, 6, Transfer::LENGTH)));
			memcpy(expect->writableData(), "<HEAD>", expect->length());
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					boost::bind(&TransferTestSuite::compareCallback, this, expect, _1));
		}
		waitFor(2);
		{
			Transfer::DenseDataPtr expect(new Transfer::DenseData(Transfer::Range(2, 12, Transfer::LENGTH)));
			memcpy(expect->writableData(), "TML>\r\n<HEAD>", expect->length());
			memory->setNext(NULL);
			memory->getData(exampleComUri,
					(Transfer::Range)*expect,
					boost::bind(&TransferTestSuite::compareCallback, this, expect, _1));
		}
		waitFor(3);
	}

};

using namespace Sirikata;
