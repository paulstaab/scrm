/*
 * scrm is an implementation of the Sequential-Coalescent-with-Recombination Model.
 * 
 * Copyright (C) 2013, 2014 Paul R. Staab, Sha (Joe) Zhu and Gerton Lunter
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
  this->tii_ = NULL;
  this->start_height_ = 0;
  this->end_height_ = 0;
}


TimeInterval::TimeInterval(TimeIntervalIterator const* tii, double start_height, double end_height){
  assert( tii != NULL );
  this->tii_ = tii;
  this->start_height_ = start_height;
  this->end_height_ = end_height;
}


// Uniformly samples a random node from the current contemporaries.
// Distribution checked.
Node* TimeInterval::getRandomContemporary(const size_t population) const {
  assert( population < forest().model().population_number() );
  assert( this->numberOfContemporaries(population) > 0 );

  // Sample the position of the Node we return
  size_t sample = forest().random_generator()->sampleInt(this->numberOfContemporaries(population));

  // Use fast random access if we only have one population 
  if ( forest().model().population_number() == 1 ) {
    return contemporaries().at(sample);
  }

  return getIthContemporaryOfPop(sample, population);
}


Node* TimeInterval::getIthContemporaryOfPop(size_t i, const size_t pop) const {
  assert( i < numberOfContemporaries(pop) );
  Node* node = NULL;
  for (size_t j = 0; j < contemporaries().size(); ++j) {
    node = contemporaries().at(j);
    assert( node != NULL );
    if ( node->population() != pop ) continue;
    if ( i == 0 ) return node;
    --i;
  }
  throw std::logic_error("Could not find the contemporary node I wanted to sample :(");
  return NULL;
}

/* --------------------------------------------------------------------
 * TimeIntervalIterator
 * -------------------------------------------------------------------*/

TimeIntervalIterator::TimeIntervalIterator() {
  this->forest_ = NULL;
  this->node_iterator_ = forest_->nodes()->iterator();
  this->good_ = true;
  this->prune = true;
  this->inside_node_ = NULL;
}


TimeIntervalIterator::TimeIntervalIterator(Forest* forest, 
                                           Node* start_node,
                                           bool prune) {
  this->forest_ = forest;
  this->good_ = true;
  this->prune = prune;
  this->inside_node_ = NULL;
  this->node_iterator_ = forest->nodes()->iterator(start_node);
  this->current_time_ = start_node->height();
  forest->writable_model()->resetTime();

  this->pop_counts_ = std::vector<size_t>(forest->model().population_number(), 0);
  
  this->contemporaries_ = std::vector<Node*>();
  this->contemporaries_.reserve(forest_->sample_size());
  this->searchContemporariesOfNode(start_node);
  
  // Skip through model changes
  while ( forest_->model().getNextTime() <= start_node->height() ) { 
    forest_->writable_model()->increaseTime();
  }

  next();
}


// Sets current_interval_ to the next time interval.
void TimeIntervalIterator::next() {
  if (this->inside_node_ != NULL ) {
    this->current_interval_.start_height_ = inside_node_->height();
    this->inside_node_ = NULL;
    return;
  }

  if (current_time_ == DBL_MAX) {
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

    // Pruning
    if (this->prune) {
        while ( !(*node_iterator_)->is_last() ) {
            // Prunes the next node BEFORE node_iterator_ gets there, 
            // and does there not invalidate it.
            if (!forest_->pruneNodeIfNeeded((*node_iterator_)->next())) break;
        }
    }

    // Move node iterator forwards
    ++node_iterator_;
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
  
  this->current_interval_ = TimeInterval(this, 
                                         start_height, 
                                         current_time_);
}


void TimeIntervalIterator::updateContemporaries(Node* current_node) {
  Node *child1 = current_node->first_child(),
       *child2 = current_node->second_child();

  // Don't add root nodes
  assert( current_node != NULL );
  if (current_node->is_root()) current_node = NULL;

  if (child1 != NULL || child2 != NULL) {
    std::vector<Node*>::iterator end = contemporaries_.end();
    for (auto it = contemporaries_.begin(); it != end; ++it) {
      // Remove first node
      if (*it == child1) {
        assert(child1 != NULL);
        if (current_node != NULL) {
          // We can just replace the node
          *it = current_node;

          ++pop_counts_.at(current_node->population());
          current_node = NULL;
        } else { 
          // We need to remove the node
          contemporaries_.erase(it--);
        }
        --pop_counts_.at(child1->population());
        child1 = NULL;
      }

      // Remove second node
      else if (*it == child2) {
        assert(child2 != NULL);
        contemporaries_.erase(it--);
        --pop_counts_.at(child2->population());
        child2 = NULL;
      }

      if (child1 == NULL && child2 == NULL) break;
    }
  }

  if (current_node != NULL) this->addToContemporaries(current_node);
}

// Removes a Node from the contemporaries if it is there.
// Currently there are situations where we try to do this also for nodes not in
// contemporaries. Maybe we could optimize this, but this may not be easy.
void TimeIntervalIterator::removeFromContemporaries(Node* node) {
  //std::cout << "Removing " << node << std::endl;
  std::vector<Node*>::iterator end = contemporaries_.end();
  for (auto it = contemporaries_.begin(); it != end; ++it) {
    if ( *it == node ) {
      contemporaries_.erase(it);
      pop_counts_.at(node->population()) -= 1;
      return;
    }
  }
}

/** 
 * Finds all nodes which code for branches at the height of a given node in the
 * tree (i.e. the node's contemporaries). Saves this nodes in the contemporaries_
 * member.
 * 
 * @param node Node for which we are searching contemporaries
 * @return Nothing. Nodes are saved in contemporaries_.
 */
void TimeIntervalIterator::searchContemporariesOfNode(Node *node) {
  if (contemporaries_.size() > 0) contemporaries_.clear();

  Node *prev, *child;

  for (NodeIterator ni = forest_->nodes()->iterator(); *ni != node; ++ni) {
    assert(ni.good());

    // Check if *ni is a contemporary of node 
    if ( (*ni)->parent_height() > node->height() ) {
      // Is is; it may however be a node we need to prune
      if ((*ni)->is_first()) prev = NULL;
      else prev = (*ni)->previous();
      child = (*ni)->first_child();

      if (forest_->pruneNodeIfNeeded(*ni)) {
        // Removing the node invalidates the ni
        if (prev == NULL) ni = forest_->nodes()->iterator();
        else ni = forest_->nodes()->iterator(prev);

        // Maybe a child of the node became a contemporary by removing the node
        // This can only happen if the node has only one child.
        if ( child != NULL && child->parent_height() > node->height() ) { 
          this->addToContemporaries(child);
        }
      } else {
        // No pruning => Just add to contemporaries
        this->addToContemporaries(*ni);
      }
    }

    // Nothing to do if its not contemporary
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
    if (contemporaries().size() > 0) contemporaries_.clear();
    current_node = this->forest_->primary_root();
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
  if (!node_iterator_.good()) {
    node_iterator_ = NodeIterator(forest_->nodes()->last());
  } 
  else {
    // Set node iterator back to the node at the current start_height
    while ( (*node_iterator_)->height() > current_interval_.start_height() ) --node_iterator_;
    ++node_iterator_;
  }

  // Then go to the next node
  current_interval_.end_height_ = (*node_iterator_)->height();
  current_time_ = (*node_iterator_)->height();
}
