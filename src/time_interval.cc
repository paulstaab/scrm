/*
 * scrm is an implementation of the Sequential-Coalescent-with-Recombination Model.
 * 
 * Copyright (C) 2013 Paul R. Staab, Sha (Joe) Zhu and Gerton Lunter
 * 
 * This file is part of scrm.
 * 
 * scrm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "time_interval.h"

/* --------------------------------------------------------------------
 * TimeInterval
 * -------------------------------------------------------------------*/

TimeInterval::TimeInterval() {
  this->forest_ = NULL;
  this->start_height_ = 0;
  this->end_height_ = 0;
  this->contemporaries_ = NULL;
}


TimeInterval::TimeInterval(Forest* forest, 
             double start_height, 
             double end_height,
             std::vector<Node*> *contemporaries) {

  assert( contemporaries != NULL );
  this->forest_ = forest;
  this->start_height_ = start_height;
  this->end_height_ = end_height;
  this->contemporaries_ = contemporaries;
}


// Uniformly samples a random node from the current contemporaries.
// Distribution checked.
Node* TimeInterval::getRandomContemporary() const {
  if ( this->numberOfContemporaries() == 0) 
    throw std::out_of_range("Error: Sampling from empty contemporaries");

  size_t sample = this->forest_->random_generator()->sampleInt(this->numberOfContemporaries());
  return contemporaries().at(sample);
}


// Checks if all nodes in contemporaries are contempoaries.
// Does not check if their are an nodes missing.
bool TimeInterval::checkContemporaries() const {
  //std::cout << "Cont: " << contemporaries_ << std::endl;
  assert( contemporaries_ != NULL );
  for (std::vector<Node*>::const_iterator it = contemporaries_->begin(); 
       it != contemporaries_->end(); ++it) {

    //std::cout << "Size " << numberOfContemporaries() << " checking " << *it << std::endl;
    if ( (*it)->height() > start_height_ || (*it)->parent_height() < end_height_ ) {
      std::cout << "Non-contemporary node " << *it << " in contemporaries" << std::endl; 
      return 0;
    }
  }
  return 1;
}




/* --------------------------------------------------------------------
 * TimeIntervalIterator
 * -------------------------------------------------------------------*/

TimeIntervalIterator::TimeIntervalIterator() {
  this->forest_ = NULL;
  this->node_iterator_ = forest_->nodes()->iterator();
  this->good_ = true;
  this->inside_node_ = NULL;
};


TimeIntervalIterator::TimeIntervalIterator(Forest* forest, 
                                           Node* start_node, 
                                           bool pruning) {
  this->forest_ = forest;
  this->good_ = true;
  this->inside_node_ = NULL;
  this->node_iterator_ = forest->nodes()->iterator(start_node);
  this->pruning_ = pruning;
  this->current_time_ = start_node->height();
  forest->writable_model()->resetTime();
  
  this->contemporaries_ = std::vector<Node*>();
  this->contemporaries_.reserve(forest_->sample_size());
  this->searchContemporariesOfNode(start_node);
  
  // Skip through model changes
  while ( forest_->model().getNextTime() <= start_node->height() ) { 
    forest_->writable_model()->increaseTime();
  }

  next();
}


// If we prune, then also prune nodes at the top of the tree
TimeIntervalIterator::~TimeIntervalIterator() {
  if (!pruning_) return;

  for( ; node_iterator_.good(); node_iterator_++ ) {
    if ( forest_->isPrunable(*node_iterator_) ) forest_->prune(*node_iterator_);
  }
}


// Sets current_interval_ to the next time interval.
void TimeIntervalIterator::next() {
  if (this->inside_node_ != NULL ) {
    this->current_interval_.start_height_ = inside_node_->height();
    this->inside_node_ = NULL;
    assert( this->current_interval_.checkContemporaries() );  
    return;
  }

  if (current_time_ == FLT_MAX) {
    good_ = false;
    return;
  }

  double start_height = this->current_time_;

  // Ensure that both iterators point into the future to determine the end of the
  // interval 
  if ( start_height >= forest_->model().getNextTime() ) { 
    forest_->writable_model()->increaseTime();
  } 
  
  if ( start_height >= node_iterator_.height() ) {
    updateContemporaries(*node_iterator_);

    // Move node iterator forwards
    ++node_iterator_;

    // Pruning
    // Maybe we could prune were allways, as we are encountering this part of
    // the tree anyway. Warning: just removing pruning_ below will not have this
    // effect.
    while ( pruning_ && node_iterator_.good() && forest_->isPrunable(*node_iterator_) ) {
      forest_->prune(node_iterator_++);
    }
  }

  double next_model_change_ = forest_->model().getNextTime();

  assert( current_time_ <= next_model_change_ );
  //std::cout << "current_time: " << current_time_ << " ni_height: " << node_iterator_.height() << std::endl;
  assert( current_time_ <= node_iterator_.height() );
 

  // Now determine the end of the interval
  if ( node_iterator_.height() <= next_model_change_ ) {
    current_time_ = node_iterator_.height();
  } else  {
    current_time_ = next_model_change_;
  }
  //std::cout << " Next Node: " << node_iterator_.height()
  //          << " Next MC: " << next_model_change_ 
  //          << " CT " << current_time_ << std::endl;

  //Don't return TimeIntervals of length zero, as nothing can happen there...
  if (start_height == current_time_) return next();
  
  this->current_interval_ = TimeInterval(this->forest_, 
                                         start_height, 
                                         current_time_, 
                                         &contemporaries_);

  assert( this->current_interval_.checkContemporaries() );  
}


void TimeIntervalIterator::updateContemporaries(Node* current_node) {
  if ( current_node->first_child() != NULL ) { 
    this->removeFromContemporaries(current_node->first_child());
    if ( current_node->second_child() != NULL ) 
      this->removeFromContemporaries(current_node->second_child());
  }

  if ( ! current_node->is_root() ) 
    this->addToContemporaries(current_node);
}

// Removes a Node from the contemporaries if it is there.
// Currently there are situations where we try to do this also for nodes not in
// contemporaries. Maybe we could optimize this, but this may not be easy.
void TimeIntervalIterator::removeFromContemporaries(Node* node) {
  //std::cout << "Removing " << node << std::endl;
  std::vector<Node*>::iterator it;
  for (it = contemporaries_.begin(); it != contemporaries_.end(); ++it) {
    //std::cout << *it << " == " << node << " ?" << std::endl;
    if ( *it == node ) {
      contemporaries_.erase(it);
      return;
    }
  }
  //throw std::logic_error("Trying to delete noexisting node from contemporaries");
}

/** 
 * Findes all nodes which code for branches at the height of a given node in the
 * tree (i.e. the node's contemporaries). Saves this nodes in the contemporaries_
 * member.
 * 
 * @param node Node for which we are searching contemporaries
 * @return Nothing. Nodes are saved in contemporaries_.
 */
void TimeIntervalIterator::searchContemporariesOfNode(Node *node) {
  if (contemporaries_.size() > 0) contemporaries_.clear();

  NodeIterator node_iterator = forest_->nodes()->iterator();
  for( ; *node_iterator != node; node_iterator++ ) {
    if ( ! node_iterator.good() )
      throw std::out_of_range("TimeIntervalIterator: start_node not found");

    if ( pruning_ && forest_->isPrunable(*node_iterator) ) {
      forest_->prune(*node_iterator);
      continue;
    }

    if ( (*node_iterator)->parent_height() > node->height() )
      this->addToContemporaries(*node_iterator);
  }
}


/** 
 * Does the same as searchContemporariesOfNode, but works its way recursively
 * down the tree.
 *
 * @param node Node for which we are searching contemporaries
 * @return Nothing. Nodes are saved in contemporaries_.
 */
void TimeIntervalIterator::searchContemporariesOfNodeTopDown(Node *node, Node *current_node) {
  // Does not work at the moment, because we do not track the roots of the other
  // trees in the forest.
  std::cout << node << " " << current_node << std::endl;
  if (current_node == NULL) {
    if (contemporaries_.size() > 0) contemporaries_.clear();
    current_node == this->forest_->primary_root();
    assert( current_node != NULL);
    std::cout << "P.root: " << current_node << std::endl;
  }
  
  /*
    if ( pruning_ && forest_->isPrunable(*node_iterator) ) {
      forest_->prune(*node_iterator);
      continue;
    }
    */

  if (current_node->height() > node->height() ) {
    if (node->first_child() != NULL) searchContemporariesOfNodeTopDown(node, node->first_child());
    if (node->second_child() != NULL) searchContemporariesOfNodeTopDown(node, node->second_child());
  } 
  else {
    if (node == current_node) return;
    this->addToContemporaries(current_node);
  }
}


// Recalculates the borders of the current interval. 
// Used after one or more nodes where created inside the interval due to events
// occurring within.
void TimeIntervalIterator::recalculateInterval() {
  // Set node iterator back to the node at the current start_height
  while ( (*node_iterator_)->height() > current_interval_.start_height() ) --node_iterator_;
  // Than go to the next node
  ++node_iterator_;
  current_interval_.end_height_ = (*node_iterator_)->height();
  current_time_ = (*node_iterator_)->height();
}
