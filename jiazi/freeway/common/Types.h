#ifndef _DFC_TYPES_H
#define _DFC_TYPES_H
#include <cstdint>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <deque>
#include <set>
#include <functional>
#include <atomic>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>
#include <boost/functional/hash.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <tbb/cache_aligned_allocator.h>
#include <boost/circular_buffer.hpp>

typedef boost::posix_time::ptime DateTime;
typedef boost::gregorian::date Date;
using namespace boost::gregorian;
typedef boost::posix_time::time_duration TimeSpan;

namespace fs = boost::filesystem;

template<typename F>
using event = boost::signals2::signal<F>;

template<typename T>
using dvector = std::vector<T, tbb::cache_aligned_allocator<T> >;

template<typename T>
using dlist = std::list<T, tbb::cache_aligned_allocator<T> >;

template<typename K, typename V, typename C = std::less<K>, typename A = tbb::cache_aligned_allocator<std::pair<const K,V> > >
using dmap = std::map<K, V, C, A>;

template<typename K, typename V, typename C = std::less<K>, typename A = tbb::cache_aligned_allocator<std::pair<const K,V> > >
using dmultimap = std::multimap<K, V, C, A>;

template<typename T, typename C = std::less<T>, typename A = tbb::cache_aligned_allocator<T> >
using dset = std::set<T, C, A>;

template<typename T, typename C = std::less<T>, typename A = tbb::cache_aligned_allocator<T> >
using dmultiset = std::multiset<T, C, A>;

template<typename K, typename V, typename H = boost::hash<K>, typename E = std::equal_to<K>, typename A = tbb::cache_aligned_allocator<std::pair<const K,V> > >
using dunordered_map = std::unordered_map<K, V, H, E, A>;

template<typename K, typename V, typename H = boost::hash<K>, typename E = std::equal_to<K>, typename A = tbb::cache_aligned_allocator<std::pair<const K,V> > >
using dunordered_multimap = std::unordered_multimap<K, V, H, E, A>;

template<typename T, typename H = boost::hash<T>, typename E = std::equal_to<T>, typename A = tbb::cache_aligned_allocator<T> >
using dunordered_set = std::unordered_set<T, H, E, A>;

template<typename T, typename H = boost::hash<T>, typename E = std::equal_to<T>, typename A = tbb::cache_aligned_allocator<T> >
using dunordered_multiset = std::unordered_multiset<T, H, E, A>;

template<typename T, typename A = tbb::cache_aligned_allocator<T>>
using ddeque = std::deque<T, A>;

template<typename T, typename A = tbb::cache_aligned_allocator<T>>
using dqueue = std::queue<T, A>;

template<typename T, typename C = ddeque<T>>
using dstack = std::stack<T, C>;

template <class T, class Container = dvector<T>,
  class Compare = std::less<typename Container::value_type> >
using dpriority_queue = std::priority_queue<T, Container, Compare>;

template<typename T>
struct consist_insert
{
    typedef typename dvector<T>::iterator vec_it;
    typedef typename dset<T>::iterator set_it;

    static void push_back(dvector<T>& vec, const T& v) { vec.push_back(v); }
    static void push_back(dset<T>& vec, const T& v) { vec.insert(v); }

    template<typename I>
    static void insert(dvector<T>& vec,I first, I last) { vec.insert(vec.end(), first, last); }

    template<typename I>
    static void insert(dset<T>& vec,I first, I last) { vec.insert(first, last); }

    static void reserve(dvector<T>& vec, size_t n) { vec.reserve(n); }
    static void reserve(dset<T>& vec, size_t n) {}
};

template<typename T>
using circular_buffer_ptr = std::shared_ptr<boost::circular_buffer<T> >;

template<typename T>
circular_buffer_ptr<T> make_circular_buffer(int n)
{
    return std::make_shared<boost::circular_buffer<T> >(n);
}
#endif
