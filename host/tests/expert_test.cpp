//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <uhd/property_tree.hpp>
#include <uhdlib/experts/expert_container.hpp>
#include <uhdlib/experts/expert_factory.hpp>
#include <fstream>

using namespace uhd::experts;

class worker1_t : public worker_node_t {
public:
    worker1_t(const node_retriever_t& db)
    : worker_node_t("A+B=C"), _a(db, "A/desired"), _b(db, "B"), _c(db, "C")
    {
        bind_accessor(_a);
        bind_accessor(_b);
        bind_accessor(_c);
    }

private:
    void resolve() {
        _c = _a + _b;
    }

    data_reader_t<int> _a;
    data_reader_t<int> _b;
    data_writer_t<int> _c;
};

//=============================================================================

class worker2_t : public worker_node_t {
public:
    worker2_t(const node_retriever_t& db)
    : worker_node_t("C*D=E"), _c(db, "C"), _d(db, "D"), _e(db, "E")
    {
        bind_accessor(_c);
        bind_accessor(_d);
        bind_accessor(_e);
    }

private:
    void resolve() {
        _e.set(_c.get() * _d.get());
    }

    data_reader_t<int> _c;
    data_reader_t<int> _d;
    data_writer_t<int> _e;
};

//=============================================================================

class worker3_t : public worker_node_t {
public:
    worker3_t(const node_retriever_t& db)
    : worker_node_t("-B=F"), _b(db, "B"), _f(db, "F")
    {
        bind_accessor(_b);
        bind_accessor(_f);
    }

private:
    void resolve() {
        _f.set(-_b.get());
    }

    data_reader_t<int> _b;
    data_writer_t<int> _f;
};

//=============================================================================

class worker4_t : public worker_node_t {
public:
    worker4_t(const node_retriever_t& db)
    : worker_node_t("E-F=G"), _e(db, "E"), _f(db, "F"), _g(db, "G")
    {
        bind_accessor(_e);
        bind_accessor(_f);
        bind_accessor(_g);
    }

private:
    void resolve() {
        _g.set(_e.get() - _f.get());
    }

    data_reader_t<int> _e;
    data_reader_t<int> _f;
    data_writer_t<int> _g;
};

//=============================================================================

class worker5_t : public worker_node_t {
public:
    worker5_t(const node_retriever_t& db, boost::shared_ptr<int> output)
    : worker_node_t("Consume_G"), _g(db, "G"), _c(db, "C"), _output(output)
    {
        bind_accessor(_g);
//        bind_accessor(_c);
    }

private:
    void resolve() {
        *_output = _g;
    }

    data_reader_t<int> _g;
    data_writer_t<int> _c;

    boost::shared_ptr<int> _output;
};

class worker6_t : public worker_node_t {
public:
    worker6_t() : worker_node_t("null_worker")
    {
    }

private:
    void resolve() {
    }
};

//=============================================================================

#define DUMP_VARS \
    BOOST_TEST_MESSAGE( str(boost::format("### State = {A=%d%s, B=%d%s, C=%d%s, D=%d%s, E=%d%s, F=%d%s, G=%d%s}\n") % \
    nodeA.get() % (nodeA.is_dirty()?"*":"") % \
    nodeB.get() % (nodeB.is_dirty()?"*":"") % \
    nodeC.get() % (nodeC.is_dirty()?"*":"") % \
    nodeD.get() % (nodeD.is_dirty()?"*":"") % \
    nodeE.get() % (nodeE.is_dirty()?"*":"") % \
    nodeF.get() % (nodeF.is_dirty()?"*":"") % \
    nodeG.get() % (nodeG.is_dirty()?"*":"")) );

#define VALIDATE_ALL_DEPENDENCIES \
    BOOST_CHECK(!nodeA.is_dirty()); \
    BOOST_CHECK(!nodeB.is_dirty()); \
    BOOST_CHECK(!nodeC.is_dirty()); \
    BOOST_CHECK(!nodeD.is_dirty()); \
    BOOST_CHECK(!nodeE.is_dirty()); \
    BOOST_CHECK(!nodeF.is_dirty()); \
    BOOST_CHECK(!nodeG.is_dirty()); \
    BOOST_CHECK(nodeC.get() == nodeA.get() + nodeB.get()); \
    BOOST_CHECK(nodeE.get() == nodeC.get() * nodeD.get()); \
    BOOST_CHECK(nodeF.get() == - nodeB.get()); \
    BOOST_CHECK(nodeG.get() == nodeE.get() - nodeF.get()); \
    BOOST_CHECK(nodeG.get() == *final_output);


BOOST_AUTO_TEST_CASE(test_experts){
    //Initialize container object
    expert_container::sptr container = expert_factory::create_container("example");
    uhd::property_tree::sptr tree = uhd::property_tree::make();

    //Output of expert tree
    boost::shared_ptr<int> final_output = boost::make_shared<int>();

    //Add data nodes to container
    expert_factory::add_dual_prop_node<int>(container, tree, "A", 0, uhd::experts::AUTO_RESOLVE_ON_WRITE);
    expert_factory::add_prop_node<int>(container, tree, "B", 0);
    expert_factory::add_data_node<int>(container, "C", 0);
    expert_factory::add_data_node<int>(container, "D", 1);
    expert_factory::add_prop_node<int>(container, tree, "E", 0, uhd::experts::AUTO_RESOLVE_ON_READ);
    expert_factory::add_data_node<int>(container, "F", 0);
    expert_factory::add_data_node<int>(container, "G", 0);

    //Add worker nodes to container
    expert_factory::add_worker_node<worker1_t>(container, container->node_retriever());
    expert_factory::add_worker_node<worker2_t>(container, container->node_retriever());
    expert_factory::add_worker_node<worker3_t>(container, container->node_retriever());
    expert_factory::add_worker_node<worker4_t>(container, container->node_retriever());
    expert_factory::add_worker_node<worker5_t>(container, container->node_retriever(), final_output);
    expert_factory::add_worker_node<worker6_t>(container);

    //Once initialized, getting modify access to graph nodes is possible (by design) but extremely red-flaggy!
    //But we do it here to monitor things
    data_node_t<int>& nodeA = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("A/desired"))));
    data_node_t<int>& nodeB = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("B"))));
    data_node_t<int>& nodeC = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("C"))));
    data_node_t<int>& nodeD = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("D"))));
    data_node_t<int>& nodeE = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("E"))));
    data_node_t<int>& nodeF = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("F"))));
    data_node_t<int>& nodeG = *(const_cast< data_node_t<int>* >(dynamic_cast< const data_node_t<int>* >(&container->node_retriever().lookup("G"))));

    DUMP_VARS

    //Ensure init behavior
    BOOST_CHECK(nodeA.is_dirty());
    BOOST_CHECK(nodeB.is_dirty());
    BOOST_CHECK(nodeC.is_dirty());
    BOOST_CHECK(nodeD.is_dirty());
    BOOST_CHECK(nodeE.is_dirty());
    BOOST_CHECK(nodeF.is_dirty());
    BOOST_CHECK(nodeG.is_dirty());
    container->resolve_all();
    VALIDATE_ALL_DEPENDENCIES       //Ensure a default resolve

    //Ensure basic node value propagation
    tree->access<int>("B").set(3);
    BOOST_CHECK(nodeB.get() == 3);  //Ensure value propagated
    BOOST_CHECK(nodeB.is_dirty());  //Ensure that nothing got resolved...
    container->resolve_all();
    VALIDATE_ALL_DEPENDENCIES

    nodeD.set(2);   //Hack for testing

    //Ensure auto-resolve on write
    tree->access<int>("A").set(200);
    BOOST_CHECK(nodeC.get() == nodeA.get() + nodeB.get());
    BOOST_CHECK(nodeE.get() == nodeC.get() * nodeD.get());
    BOOST_CHECK(nodeG.get() == nodeE.get() - nodeF.get());
    container->resolve_all();
    VALIDATE_ALL_DEPENDENCIES

    container->resolve_all();
    VALIDATE_ALL_DEPENDENCIES

    //Ensure auto-resolve on read
    tree->access<int>("E").get();
    BOOST_CHECK(nodeC.get() == nodeA.get() + nodeB.get());
    BOOST_CHECK(nodeE.get() == nodeC.get() * nodeD.get());
    BOOST_CHECK(!nodeE.is_dirty());
    tree->access<int>("E").set(-10);
    container->resolve_all(true);
    VALIDATE_ALL_DEPENDENCIES

    //Resolve to/from
    tree->access<int>("A").set(-1);
    container->resolve_to("C");
    BOOST_CHECK(nodeC.get() == nodeA.get() + nodeB.get());
    BOOST_CHECK(!nodeC.is_dirty());
    container->resolve_to("Consume_G");
    VALIDATE_ALL_DEPENDENCIES
}
