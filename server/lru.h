#pragma once

#ifndef __S3_SERVER_LRU_H__
#define __S3_SERVER_LRU_H__

#include <utility>
#include <string>
#include <iostream>
#include <unordered_map>
#include <list>

using namespace std;

template<typename key, typename val> class lru {
	int                                                         size;
	list<key>                                                   queue;
	unordered_map<key, pair<val, typename list<key>::iterator>> map;
public:
	void reclaim() {
		while (map.size() >= size) {
			map.erase(queue.back());
			queue.pop_back();
		}
	}

	lru(int s) {
		size = s;
	}

	void put(const key k, const val v) {
		auto it = map.find(k);
		reclaim();
		if (it != map.end()) {
			queue.erase(it->second.second);
		}
		queue.push_front(k);
		map[k] = { v, queue.begin() };
	}

	val get(const key k) {
		auto it = map.find(k);
		if (it != map.end()) {
			queue.erase(it->second.second);
			queue.push_front(k);
			map[k] = { it->second.first, queue.begin() };
			return it->second.first;
		} else {
			return val();
		}
	}
	friend ostream &operator<<(ostream &out, const lru<key, val> &self) {
		string sep = "";
		out << "[";
		for (auto const& k : self.queue) {
			out << sep << k << " : " << self.map.find(k)->first;
			sep = ", ";
		}
		return out << "]";
	}
};

/* __S3_SERVER_LRU_H__ */
#endif
