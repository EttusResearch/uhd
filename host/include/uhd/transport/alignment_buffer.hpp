//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP
#define INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP

#include <uhd/config.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>
#include <vector>

namespace uhd{ namespace transport{

    /*!
     * Imlement a templated alignment buffer:
     * Used for aligning asynchronously pushed elements with matching ids.
     */
    template <typename elem_type, typename seq_type> class alignment_buffer{
    public:
        typedef boost::shared_ptr<alignment_buffer<elem_type, seq_type> > sptr;

        /*!
         * Create the alignment buffer.
         * \param capacity the maximum elements per index
         * \param width the number of elements to align
         */
        alignment_buffer(size_t capacity, size_t width){
            _buffs.resize(width);
            for (size_t i = 0; i < width; i++){
                _buffs[i].buff = bounded_buffer_sptr(new bounded_buffer_type(capacity));
                _buffs[i].has_popped_element = false;
            }
        }

        /*!
         * Destroy this alignment buffer.
         */
        ~alignment_buffer(void){
            /* NOP */
        }

        /*!
         * Push a single element into the buffer specified by index.
         * Notify the condition variable for a thread blocked in pop.
         * \param elem the element to push
         * \param seq the sequence identifier
         * \param index the buffer index
         */
        void push_elem_with_wait(const elem_type &elem, const seq_type &seq, size_t index){
            _buffs[index].buff.push_with_wait(buff_contents_type(elem, seq));
            _pushed_cond.notify_one();
        }

        /*!
         * Pop an aligned set of elements from this alignment buffer.
         * \param elems a collection to store the aligned elements
         */
        template <typename elems_type>
        void pop_elems_with_wait(elems_type &elems){
            //TODO................................
            buff_contents_type buff_contents_tmp;
            for (size_t i = 0; i < _buffs.size();){
                if (_buffs[i].has_popped_element){
                    i++:
                    continue;
                }
                _buffs[i].pop_with_wait(buff_contents_tmp);
                if (buff_contents_tmp.second == _expected_seq_id){
                    _buffs[i].has_popped_element = true;
                    i++;
                    continue;
                }

                //if the sequence number is older, pop until we get the current sequence number
                //do this by setting has popped element false and continuing on the same condition
                if (buff_contents_tmp.second < _expected_seq_id){
                    _buffs[i].has_popped_element = false;
                    continue;
                }

                //if the sequence number is newer, start from scratch at the new sequence number
                //do this by setting all has popped elements false and restarting on index zero
                if (buff_contents_tmp.second > _expected_seq_id){
                    _expected_seq_id = buff_contents_tmp.second;
                    for (size_t j = 0; j < i; j++){
                        _buffs[j].has_popped_element = false;
                    }
                    i = 0;
                    continue;
                }
            }
            //if aligned
            for (size_t i = 0; i < _buffs.size(); i++){
                elems[i] = _buffs[i].popped_element;
                _buffs[i].has_popped_element = false;
            }
        }

    private:
        //a vector of bounded buffers for each index
        typedef std::pair<elem_type, seq_type> buff_contents_type;
        typedef bounded_buffer<buff_contents_type> bounded_buffer_type;
        typedef boost::shared_ptr<bounded_buffer_type> bounded_buffer_sptr;
        struct buff_type{
            bounded_buffer_sptr buff;
            elem_type popped_element;
            bool has_popped_element;
        };
        std::vector<buff_type> _buffs;

        //the seq identifier to align with
        seq_type _expected_seq_id;

        //a condition to notify when a new element is pushed
        boost::condition_variable _pushed_cond;
    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP */
