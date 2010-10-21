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

#ifndef INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_IPP
#define INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_IPP

#include <uhd/transport/bounded_buffer.hpp>
#include <boost/thread/condition_variable.hpp>
#include <utility>

namespace uhd{ namespace transport{ namespace{ /*anon*/

    /*!
     * Imlement a templated alignment buffer:
     * Used for aligning asynchronously pushed elements with matching ids.
     */
    template <typename elem_type, typename seq_type>
    class alignment_buffer_impl : public alignment_buffer<elem_type, seq_type>{
    public:

        alignment_buffer_impl(size_t capacity, size_t width) : _last_seqs(width){
            for (size_t i = 0; i < width; i++){
                _buffs.push_back(bounded_buffer<buff_contents_type>::make(capacity));
                _all_indexes.push_back(i);
            }
            _there_was_a_clear = false;
        }

        UHD_INLINE bool push_with_pop_on_full(
            const elem_type &elem, const seq_type &seq, size_t index
        ){
            //clear the buffer for this index if the seqs are mis-ordered
            if (seq < _last_seqs[index]){
                _buffs[index]->clear();
                _there_was_a_clear = true;
            } _last_seqs[index] = seq;
            return _buffs[index]->push_with_pop_on_full(buff_contents_type(elem, seq));
        }

        UHD_INLINE bool pop_elems_with_timed_wait(
            std::vector<elem_type> &elems, double timeout
        ){
            boost::system_time exit_time = boost::get_system_time() + to_time_dur(timeout);
            buff_contents_type buff_contents_tmp;
            std::list<size_t> indexes_to_do(_all_indexes);

            //do an initial pop to load an initial sequence id
            size_t index = indexes_to_do.front();
            if (not _buffs[index]->pop_with_timed_wait(
                buff_contents_tmp, from_time_dur(exit_time - boost::get_system_time())
            )) return false;
            elems[index] = buff_contents_tmp.first;
            seq_type expected_seq_id = buff_contents_tmp.second;
            indexes_to_do.pop_front();

            //get an aligned set of elements from the buffers:
            while(indexes_to_do.size() != 0){

                //respond to a clear by starting from scratch
                if(_there_was_a_clear){
                    _there_was_a_clear = false;
                    indexes_to_do = _all_indexes;
                    index = indexes_to_do.front();
                    if (not _buffs[index]->pop_with_timed_wait(
                        buff_contents_tmp, from_time_dur(exit_time - boost::get_system_time())
                    )) return false;
                    elems[index] = buff_contents_tmp.first;
                    expected_seq_id = buff_contents_tmp.second;
                    indexes_to_do.pop_front();
                }

                //pop an element off for this index
                index = indexes_to_do.front();
                if (not _buffs[index]->pop_with_timed_wait(
                    buff_contents_tmp, from_time_dur(exit_time - boost::get_system_time())
                )) return false;

                //if the sequence id matches:
                //  store the popped element into the output,
                //  remove this index from the list and continue
                if (buff_contents_tmp.second == expected_seq_id){
                    elems[index] = buff_contents_tmp.first;
                    indexes_to_do.pop_front();
                    continue;
                }

                //if the sequence id is older:
                //  continue with the same index to try again
                if (buff_contents_tmp.second < expected_seq_id){
                    continue;
                }

                //if the sequence id is newer:
                //  store the popped element into the output,
                //  add all other indexes back into the list
                if (buff_contents_tmp.second > expected_seq_id){
                    elems[index] = buff_contents_tmp.first;
                    expected_seq_id = buff_contents_tmp.second;
                    indexes_to_do = _all_indexes;
                    indexes_to_do.remove(index);
                    continue;
                }
            }
            return true;
        }

    private:
        //a vector of bounded buffers for each index
        typedef std::pair<elem_type, seq_type> buff_contents_type;
        std::vector<typename bounded_buffer<buff_contents_type>::sptr> _buffs;
        std::vector<seq_type> _last_seqs;
        std::list<size_t> _all_indexes;
        bool _there_was_a_clear;
    };

}}} //namespace

namespace uhd{ namespace transport{

    template <typename elem_type, typename seq_type>
    typename alignment_buffer<elem_type, seq_type>::sptr
    alignment_buffer<elem_type, seq_type>::make(size_t capacity, size_t width){
        return typename alignment_buffer<elem_type, seq_type>::sptr(
            new alignment_buffer_impl<elem_type, seq_type>(capacity, width)
        );
    }

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_ALIGNMENT_BUFFER_IPP */
