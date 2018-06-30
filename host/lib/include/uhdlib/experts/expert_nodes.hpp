//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_EXPERTS_EXPERT_NODES_HPP
#define INCLUDED_UHD_EXPERTS_EXPERT_NODES_HPP

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/dirty_tracked.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/units/detail/utility.hpp>
#include <memory>
#include <list>
#include <stdint.h>

namespace uhd { namespace experts {

    enum node_class_t  { CLASS_WORKER, CLASS_DATA, CLASS_PROPERTY };
    enum node_access_t { ACCESS_READER, ACCESS_WRITER };
    enum node_author_t { AUTHOR_NONE, AUTHOR_USER, AUTHOR_EXPERT };

    /*!---------------------------------------------------------
     * class dag_vertex_t
     *
     * This serves as the base class for all nodes in the expert
     * graph. Data nodes and workers are derived from this class.
     * ---------------------------------------------------------
     */
    class dag_vertex_t : private boost::noncopyable {
    public:
        typedef boost::function<void(std::string)> callback_func_t;

        virtual ~dag_vertex_t() {}

        // Getters for basic info about the node
        inline node_class_t get_class() const {
            return _class;
        }

        inline const std::string& get_name() const {
            return _name;
        }

        virtual const std::string& get_dtype() const = 0;

        virtual std::string to_string() const = 0;

        // Graph resolution specific
        virtual bool is_dirty() const = 0;
        virtual void mark_clean() = 0;
        virtual void resolve() = 0;

        // External callbacks
        virtual void set_write_callback(const callback_func_t& func) = 0;
        virtual bool has_write_callback() const = 0;
        virtual void clear_write_callback() = 0;
        virtual void set_read_callback(const callback_func_t& func) = 0;
        virtual bool has_read_callback() const = 0;
        virtual void clear_read_callback() = 0;

    protected:
        dag_vertex_t(const node_class_t c, const std::string& n):
            _class(c), _name(n) {}

    private:
        const node_class_t  _class;
        const std::string   _name;
    };

    class data_node_printer {
    public:
        //Generic implementation
        template<typename data_t>
        static std::string print(const data_t& val) {
            std::ostringstream os;
            os << val;
            return os.str();
        }

        static std::string print(const uint8_t& val) {
            std::ostringstream os;
            os << int(val);
            return os.str();
        }

        static std::string print(const time_spec_t time) {
            std::ostringstream os;
            os << time.get_real_secs();
            return os.str();
        }
    };

    /*!---------------------------------------------------------
     * class data_node_t
     *
     * The data node class hold a passive piece of data in the
     * expert graph. A data node is clean if its underlying data
     * is clean. Access to the underlying data is provided using
     * two methods:
     * 1. Special accessor classes (for R/W enforcement)
     * 2. External clients (via commit and retrieve). This access
     *    is protected by the callback mutex.
     *
     * Requirements for data_t
     * - Must have a default constructor
     * - Must have a copy constructor
     * - Must have an assignment operator (=)
     * - Must have an equality operator (==)
     * ---------------------------------------------------------
     */
    template<typename data_t>
    class data_node_t : public dag_vertex_t {
    public:
        // A data_node_t instance can have a type of CLASS_DATA or CLASS_PROPERTY
        // In general a data node is a property if it can be accessed and modified
        // from the outside world (of experts) using read and write callbacks. We
        // assume that if a callback mutex is passed into the data node that it will
        // be accessed from the outside and tag the data node as a PROPERTY.
        data_node_t(const std::string& name, boost::recursive_mutex* mutex = NULL) :
            dag_vertex_t(mutex?CLASS_PROPERTY:CLASS_DATA, name), _callback_mutex(mutex), _data(), _author(AUTHOR_NONE) {}

        data_node_t(const std::string& name, const data_t& value, boost::recursive_mutex* mutex = NULL) :
            dag_vertex_t(mutex?CLASS_PROPERTY:CLASS_DATA, name), _callback_mutex(mutex), _data(value), _author(AUTHOR_NONE) {}

        // Basic info
        virtual const std::string& get_dtype() const {
            static const std::string dtype(
                boost::units::detail::demangle(typeid(data_t).name()));
            return dtype;
        }

        virtual std::string to_string() const {
            return data_node_printer::print(get());
        }

        inline node_author_t get_author() const {
            return _author;
        }

        // Graph resolution specific
        virtual bool is_dirty() const {
            return _data.is_dirty();
        }

        virtual void mark_clean() {
            _data.mark_clean();
        }

        void resolve() {
            //NOP
        }

        // Data node specific setters and getters (for the framework)
        void set(const data_t& value) {
            _data = value;
            _author = AUTHOR_EXPERT;
        }

        const data_t& get() const {
            return _data;
        }

        // Data node specific setters and getters (for external entities)
        void commit(const data_t& value) {
            if (_callback_mutex == NULL) throw uhd::assertion_error("node " + get_name() + " is missing the callback mutex");
            boost::lock_guard<boost::recursive_mutex> lock(*_callback_mutex);
            set(value);
            _author = AUTHOR_USER;
            if (is_dirty() and has_write_callback()) {
                _wr_callback(std::string(get_name()));  //Put the name on the stack before calling
            }
        }

        const data_t retrieve() const {
            if (_callback_mutex == NULL) throw uhd::assertion_error("node " + get_name() + " is missing the callback mutex");
            boost::lock_guard<boost::recursive_mutex> lock(*_callback_mutex);
            if (has_read_callback()) {
                _rd_callback(std::string(get_name()));
            }
            return get();
        }

    private:
        // External callbacks
        virtual void set_write_callback(const callback_func_t& func) {
            _wr_callback = func;
        }

        virtual bool has_write_callback() const {
            return not _wr_callback.empty();
        }

        virtual void clear_write_callback() {
            _wr_callback.clear();
        }

        virtual void set_read_callback(const callback_func_t& func) {
            _rd_callback = func;
        }

        virtual bool has_read_callback() const {
            return not _rd_callback.empty();
        }

        virtual void clear_read_callback() {
            _rd_callback.clear();
        }

        boost::recursive_mutex* _callback_mutex;
        callback_func_t         _rd_callback;
        callback_func_t         _wr_callback;
        dirty_tracked<data_t>   _data;
        node_author_t           _author;
    };

    /*!---------------------------------------------------------
     * class node_retriever_t
     *
     * Node storage is managed by a framework class so we need
     * and interface to find and retrieve data nodes to associate
     * with accessors.
     * ---------------------------------------------------------
     */
    class node_retriever_t {
    public:
        virtual ~node_retriever_t() {}
        virtual const dag_vertex_t& lookup(const std::string& name) const = 0;
    private:
        friend class data_accessor_t;
        virtual dag_vertex_t& retrieve(const std::string& name) const = 0;
    };

    /*!---------------------------------------------------------
     * class data_accessor_t
     *
     * Accessors provide protected access to data nodes and help
     * establish dependency relationships.
     * ---------------------------------------------------------
     */
    class data_accessor_t {
    public:
        virtual ~data_accessor_t() {}

        virtual bool is_reader() const = 0;
        virtual bool is_writer() const = 0;
        virtual dag_vertex_t& node() const = 0;
    protected:
        data_accessor_t(const node_retriever_t& r, const std::string& n):
            _vertex(r.retrieve(n)) {}
        dag_vertex_t& _vertex;
    };

    template<typename data_t>
    class data_accessor_base : public data_accessor_t {
    public:
        virtual ~data_accessor_base() {}

        virtual bool is_reader() const {
            return _access == ACCESS_READER;
        }

        virtual bool is_writer() const {
            return _access == ACCESS_WRITER;
        }

        inline bool is_dirty() const {
            return _datanode->is_dirty();
        }

        inline node_class_t get_class() const {
            return _datanode->get_class();
        }

        inline node_author_t get_author() const {
            return _datanode->get_author();
        }

    protected:
        data_accessor_base(
            const node_retriever_t& r, const std::string& n, const node_access_t a) :
                data_accessor_t(r, n), _datanode(NULL), _access(a)
        {
            _datanode = dynamic_cast< data_node_t<data_t>* >(&node());
            if (_datanode == NULL) {
                throw uhd::type_error("Expected data type for node " + n +
                                      " was " + boost::units::detail::demangle(typeid(data_t).name()) +
                                      " but got " + node().get_dtype());
            }
        }

        data_node_t<data_t>*    _datanode;
        const node_access_t     _access;

    private:
        virtual dag_vertex_t& node() const {
            return _vertex;
        }
    };

    /*!---------------------------------------------------------
     * class data_reader_t
     *
     * Accessor to read the value of a data node and to establish
     * a data node => worker node dependency
     * ---------------------------------------------------------
     */
    template<typename data_t>
    class data_reader_t : public data_accessor_base<data_t> {
    public:
        data_reader_t(const node_retriever_t& retriever, const std::string& node) :
            data_accessor_base<data_t>(
                retriever, node, ACCESS_READER) {}

        inline const data_t& get() const {
            return data_accessor_base<data_t>::_datanode->get();
        }

        inline operator const data_t&() const {
            return get();
        }

        inline bool operator==(const data_t& rhs) {
            return get() == rhs;
        }

        inline bool operator!=(const data_t& rhs) {
            return !(get() == rhs);
        }
    };

    /*!---------------------------------------------------------
     * class data_reader_t
     *
     * Accessor to read and write the value of a data node and
     * to establish a worker node => data node dependency
     * ---------------------------------------------------------
     */
    template<typename data_t>
    class data_writer_t : public data_accessor_base<data_t> {
    public:
        data_writer_t(const node_retriever_t& retriever, const std::string& node) :
            data_accessor_base<data_t>(
                retriever, node, ACCESS_WRITER) {}

        inline const data_t& get() const {
            return data_accessor_base<data_t>::_datanode->get();
        }

        inline operator const data_t&() const {
            return get();
        }

        inline bool operator==(const data_t& rhs) {
            return get() == rhs;
        }

        inline bool operator!=(const data_t& rhs) {
            return !(get() == rhs);
        }

        inline void set(const data_t& value) {
            data_accessor_base<data_t>::_datanode->set(value);
        }

        inline data_writer_t<data_t>& operator=(const data_t& value) {
            set(value);
            return *this;
        }

        inline data_writer_t<data_t>& operator=(const data_writer_t<data_t>& value) {
            set(value.get());
            return *this;
        }
};

    /*!---------------------------------------------------------
     * class worker_node_t
     *
     * A node class to implement a function that consumes
     * zero or more input data nodes and emits zero or more output
     * data nodes. The worker can also operate on other non-expert
     * interfaces because worker_node_t is abstract and the client
     * is required to implement the "resolve" method in a subclass.
     * ---------------------------------------------------------
     */
    class worker_node_t : public dag_vertex_t {
    public:
        worker_node_t(const std::string& name) :
            dag_vertex_t(CLASS_WORKER, name) {}

        // Worker node specific
        std::list<std::string> get_inputs() const {
            std::list<std::string> retval;
            for(data_accessor_t* acc:  _inputs) {
                retval.push_back(acc->node().get_name());
            }
            return retval;
        }

        std::list<std::string> get_outputs() const {
            std::list<std::string> retval;
            for(data_accessor_t* acc:  _outputs) {
                retval.push_back(acc->node().get_name());
            }
            return retval;
        }

    protected:
        // This function is used to bind data accessors
        // to this worker. Accessors can be read/write
        // and the binding will ensure proper dependency
        // handling.
        void bind_accessor(data_accessor_t& accessor) {
            if (accessor.is_reader()) {
                _inputs.push_back(&accessor);
            } else if (accessor.is_writer()) {
                _outputs.push_back(&accessor);
            } else {
                throw uhd::assertion_error("Invalid accessor type");
            }
        }

    private:
        // Graph resolution specific
        virtual bool is_dirty() const {
            bool inputs_dirty = false;
            for(data_accessor_t* acc:  _inputs) {
                inputs_dirty |= acc->node().is_dirty();
            }
            return inputs_dirty;
        }

        virtual void mark_clean() {
            for(data_accessor_t* acc:  _inputs) {
                acc->node().mark_clean();
            }
        }

        virtual void resolve() = 0;

        // Basic type info
        virtual const std::string& get_dtype() const {
            static const std::string dtype = "<worker>";
            return dtype;
        }

        virtual std::string to_string() const {
            return "<worker>";
        }

        // Workers don't have callbacks so implement stubs
        virtual void set_write_callback(const callback_func_t&) {}
        virtual bool has_write_callback() const { return false; }
        virtual void clear_write_callback() {}
        virtual void set_read_callback(const callback_func_t&) {}
        virtual bool has_read_callback() const { return false; }
        virtual void clear_read_callback() {}

        std::list<data_accessor_t*> _inputs;
        std::list<data_accessor_t*> _outputs;
    };

}}

#endif /* INCLUDED_UHD_EXPERTS_EXPERT_NODE_HPP */
