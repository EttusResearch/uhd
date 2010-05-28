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
            for (size_t i = 0; i < width; i++){
                _buffs.push_back(bounded_buffer_sptr(new bounded_buffer_type(capacity)));
                _all_indexes.push_back(i);
            }
        }

        /*!
         * Destroy this alignment buffer.
         */
        ~alignment_buffer(void){
            /* NOP */
        }

        /*!
         * Push an element with sequence id into the buffer at index.
         * \param elem the element to push
         * \param seq the sequence identifier
         * \param index the buffer index
         * \return true if the element fit without popping for space
         */
        UHD_INLINE bool push_with_pop_on_full(
            const elem_type &elem,
            const seq_type &seq,
            size_t index
        ){
            return _buffs[index]->push_with_pop_on_full(buff_contents_type(elem, seq));
        }

        /*!
         * Pop an aligned set of elements from this alignment buffer.
         * \param elems a collection to store the aligned elements
         * \param time the timeout time
         * \return false when the operation times out
         */
        template <typename elems_type, typename time_type>
        bool pop_elems_with_timed_wait(elems_type &elems, const time_type &time){
            buff_contents_type buff_contents_tmp;
            std::list<size_t> indexes_to_do(_all_indexes);

            //the seq identifier to align with
            seq_type expected_seq_id = seq_type();
            bool expected_seq_id_valid = false;

            //get an aligned set of elements from the buffers:
            while(indexes_to_do.size() != 0){
                size_t index = indexes_to_do.back();

                //pop an element off for this index
                if (not _buffs[index]->pop_with_timed_wait(buff_contents_tmp, time)) return false;

                //grab the current sequence id if not valid
                if (not expected_seq_id_valid){
                    expected_seq_id_valid = true;
                    expected_seq_id = buff_contents_tmp.second;
                }

                //if the sequence id matches:
                //  store the popped element into the output,
                //  remove this index from the list and continue
                if (buff_contents_tmp.second == expected_seq_id){
                    elems[index] = buff_contents_tmp.first;
                    indexes_to_do.pop_back();
                    continue;
                }

                //if the sequence id is older:
                //  continue with the same index to try again
                if (buff_contents_tmp.second < expected_seq_id){
                    continue;
                }

                //if the sequence id is newer:
                //  start from scratch at the new sequence number
                if (buff_contents_tmp.second > expected_seq_id){
                    expected_seq_id = buff_contents_tmp.second;
                    indexes_to_do = _all_indexes;
                    continue;
                }
            }
            return true;
        }

    private:
        //a vector of bounded buffers for each index
        typedef std::pair<elem_type, seq_type> buff_contents_type;
        typedef bounded_buffer<buff_contents_type> bounded_buffer_type;
        typedef boost::shared_ptr<bounded_buffer_type> bounded_buffer_sptr;
        std::vector<bounded_buffer_sptr> _buffs;
        std::list<size_t> _all_indexes;
    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_HPP */
