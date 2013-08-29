/* A priority queue for the Image Forest Transform. Special
* requirement is to be able to efficiently remove specific elements.
* There are a number of conflicting requirements - I'm not sure what
* is going to be the best approach.
* 1) Two maps, One keyed by priority, one keyed by index.
* 2) The old style FAH, plus a map or set storing position and
* priority. Look up the priority and then do a linear search for
* matching index.
* 3) Index vs offset. Probably going to much quicker for these
* structures to use a single valued offset, but ITK doesn't let us
* access pixels that way. Need to convert back and forth.



*/

#ifndef _itk_IFTQueue_h_
#define _itk_IFTQueue_h_

#include <map>
#include <list>
#include <iostream>

template< typename TKey, typename TValue, typename TKeyComp=std::less<TKey>, typename TValueComp=std::less<TValue> >
class IFTQueueA {
public:
  // this is modified from "mutable_priority_queue" that I found
  // somewhere. Difference is the use of multimap. This approach
  // should be OK for the way I plan to use it in the IFT. Choice of a
  // key including an iteration number leads to unique keys. This is
  // different to the map of queues used in the standard watershed implementation.
  // Also, values are image locations, which are unique, and therefore
  // we don't need the multimap for the reverse lookup.

  typedef typename std::map<TKey,TValue,TKeyComp>::const_iterator iterator;
  typedef typename std::map<TKey,TValue,TKeyComp>::const_iterator const_iterator;

  // default constructor, uses std::greater and std::less for key and value comparisons
  IFTQueueA() : KeyMap(TKeyComp()), ValueMap(TValueComp()) { }

  // constructor with key comparison predicate
  IFTQueueA( const TKeyComp &keyComp ) : KeyMap(TKeyComp()), ValueMap(TValueComp()){ }

  // constructor with value comparison predicate
  IFTQueueA( const TValueComp &valComp ) : KeyMap(TKeyComp()), ValueMap(valComp) { }

  // constructor with both comparison predicates
  IFTQueueA( const TKeyComp &keyComp, const TValueComp &valComp ) : KeyMap(keyComp), ValueMap(valComp) { }

  // copy constructor
  IFTQueueA( const IFTQueueA<TKey,TValue,TKeyComp,TValueComp> &x ) : KeyMap(x.KeyMap), ValueMap(x.ValueMap) {}

  // destructor, clears both maps
  ~IFTQueueA(){
    KeyMap.clear();
    ValueMap.clear();
  }

  // assignment operator
  inline void operator=( IFTQueueA<TKey,TValue,TKeyComp,TValueComp> x ){
    KeyMap = x.KeyMap;
    ValueMap = x.ValueMap;
  }

  // returns an iterator for the front of the queue
  inline iterator begin(){ 
    return KeyMap.begin();
  }

  // returns an iterator for the end of the queue
  inline iterator end(){
    return KeyMap.end();
  }

  // empties the queue
  inline void clear(){
    KeyMap.clear();
    ValueMap.clear();
  }

  // removes from both maps the value val
  inline void erase( TValue val ){
    typename std::map<TValue,TKey,TValueComp>::iterator iter=ValueMap.find( val );
    if( iter != ValueMap.end() ){
    KeyMap.erase( KeyMap.find(iter->second) );
    ValueMap.erase( iter );
    }
  }

  // returns true if the queue is empty
  inline bool empty(){
    return KeyMap.empty();
  }

  // returns the value at the front of the queue
  inline TValue front_value(){
    return (KeyMap.begin())->second;
  }

  // returns the key at the front of the queue
  inline TKey front_key(){
    return (KeyMap.begin())->first;
  }

  // removes the front entry in the queue
  inline void pop(){
    iterator iter = KeyMap.begin();
    sperase( iter );
  }

  // push an entry onto the queue
  inline void push( TValue val, TKey key ){
    insert( val, key );
  }

  // adds a value key pair to the queue.
  // an attempt first has to be made to erase
  // the item, since it could exist with another
  // key and the queue does not allow multiple
  // keys per value
  inline void insert( TValue val, TKey key ){
    erase( val );
    KeyMap.insert( std::pair<TKey,TValue>( key, val ) );
    ValueMap.insert( std::pair<TValue,TKey>( val, key ) );
  }

  // update the key associated with a value,
  // simply a convenience function that calls
  // insert()
  inline void update( TValue val, TKey key ){
    insert( val, key );
  }

  // returns the number of elements in the queue
  inline size_t size(){
    return KeyMap.size();
  }

  void PrintKeyMap()
  {
    for (iterator kit = KeyMap.begin(); kit != KeyMap.end();++kit)
      {
      std::cout << kit->first << " " << kit->second << std::endl;
      }
  }

private:
  // Keys will be priorities, Values be image locations, in some form.
  // Priorities will be gray level/iteration number pairs
  std::map<TKey, TValue, TKeyComp> KeyMap;  
  std::map<TValue, TKey, TValueComp> ValueMap;

  inline void sperase( typename std::map<TKey,TValue,TKeyComp>::iterator it ){
    ValueMap.erase( it->second );
    KeyMap.erase( it );
  }

};

template< typename TKey, typename TValue, typename TKeyComp=std::less<TKey>, typename TValueComp=std::less<TValue> >
class IFTQueueB {
private:
  // Keys will be priorities, Values be image locations, in some form.
  // Priorities will be gray level/iteration number pairs
  // normally this would
  // be a queue, but we
  // need to erase elements
  typedef typename std::list<TValue> ListType; 
  std::map<TKey, ListType, TKeyComp> KeyMap;  
  std::map<TValue, TKey, TValueComp> ValueMap;

  typedef typename std::map<TKey, ListType, TKeyComp>::iterator iterator;
  typedef typename std::map<TValue, TKey, TValueComp>::const_iterator const_iterator;

  TValueComp valComp;

public:
  // this version is derived from the hierarchical queue used in the
  // watershed that preserves the order in which entries of the same
  // priority (Key) are added. We'll use a linear search to find the
  // particular value we want to get rid of 

  // default constructor, uses std::greater and std::less for key and value comparisons
  IFTQueueB() : KeyMap(TKeyComp()), ValueMap(TValueComp()) { }

  // constructor with key comparison predicate
  IFTQueueB( const TKeyComp &keyComp ) : KeyMap(TKeyComp()), ValueMap(TValueComp()){ }

  // constructor with value comparison predicate
  IFTQueueB( const TValueComp &valComp ) : KeyMap(TKeyComp()), ValueMap(TValueComp()) { }

  // constructor with both comparison predicates
  IFTQueueB( const TKeyComp &keyComp, const TValueComp &valComp ) : KeyMap(keyComp), ValueMap(valComp) { }

  // copy constructor
  IFTQueueB( const IFTQueueB<TKey,TValue,TKeyComp,TValueComp> &x ) : KeyMap(x.KeyMap), ValueMap(x.ValueMap) {}

  // destructor, clears both maps
  ~IFTQueueB(){
    KeyMap.clear();
    ValueMap.clear();
  }

  // assignment operator
  inline void operator=( IFTQueueB<TKey,TValue,TKeyComp,TValueComp> x ){
    KeyMap = x.KeyMap;
    ValueMap = x.ValueMap;
  }

  // returns an iterator for the front of the queue
  inline iterator begin(){ 
    return KeyMap.begin();
  }

  // returns an iterator for the end of the queue
  inline iterator end(){
    return KeyMap.end();
  }

  // empties the queue
  inline void clear(){
    KeyMap.clear();
    ValueMap.clear();
  }

  // removes from both maps the value val
  inline void erase( TValue val ){
    typename std::map<TValue,TKey,TValueComp>::iterator iter=ValueMap.find( val );
    if( iter != ValueMap.end() )
      {
      typename std::map<TKey, ListType, TKeyComp>::iterator i2 = KeyMap.find(iter->second);
      typename std::list<TValue>::iterator lit;
      for (lit = i2->second.begin(); lit != i2->second.end(); ++lit)
	{
	if ((!valComp(*lit, val)) && (!valComp(val, *lit))) 
	  {
	  i2->second.erase(lit);
	  } 
	}
      ValueMap.erase( iter );
      }
  }

  // returns true if the queue is empty
  inline bool empty(){
    return KeyMap.empty();
  }

  // returns the value at the front of the queue
  inline TValue front_value(){
    return (*(KeyMap.begin())->second.begin());
  }

  // returns the key at the front of the queue
  inline TKey front_key(){
    return (KeyMap.begin())->first;
  }

  // removes the front entry in the queue
  inline void pop(){
    iterator iter = KeyMap.begin();
    sperase( iter );
  }

  // push an entry onto the queue
  inline void push( TValue val, TKey key ){
    insert( val, key );
  }

  // adds a value key pair to the queue.
  // an attempt first has to be made to erase
  // the item, since it could exist with another
  // key and the queue does not allow multiple
  // keys per value
  inline void insert( TValue val, TKey key ){
    erase( val );
    KeyMap[key].push_back(val);
    ValueMap.insert( std::pair<TValue,TKey>( val, key ) );
  }

  // might need to have a special insert for when we are sure that the
  // value isn't already there
  

  // update the key associated with a value,
  // simply a convenience function that calls
  // insert()
  inline void update( TValue val, TKey key ){
    insert( val, key );
  }

  // returns the number of elements in the queue
  inline size_t size(){
    return KeyMap.size();
  }

  void PrintKeyMap()
  {
    for (iterator kit = KeyMap.begin(); kit != KeyMap.end();++kit)
      {
      std::cout << kit->first << " ";
      typename std::list<TValue>::iterator lit;
      for (lit = kit->second.begin(); lit != kit->second.end(); ++lit)
	{
	std::cout << *lit << " " ;

	}
      std::cout << std::endl;
      }
  }

private:
  // Keys will be priorities, Values be image locations, in some form.
  // Priorities will be gray level/iteration number pairs
  // normally this would
  // be a queue, but we
  // need to erase elements
  // typedef typename std::list<TValue> ListType; 
  // std::map<TKey, ListType, TKeyComp> KeyMap;  
  // std::map<TValue, TKey, TValueComp> ValueMap;

  // typedef typename KeyMap::iterator iterator;
  // typedef typename ValueMap::const_iterator const_iterator;


  inline void sperase( typename std::map<TKey,ListType,TKeyComp>::iterator it ){
    ValueMap.erase( it->second );
    KeyMap.erase( it );
  }

};


#endif
