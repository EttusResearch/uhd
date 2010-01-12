//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/wax.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <boost/assert.hpp>

class wax_demo : public wax::obj{
private:
    std::vector<float> d_nums;
    std::vector<wax_demo> d_subs;
public:
    wax_demo(size_t sub_demos, size_t len){
        d_nums = std::vector<float>(len);
        if (sub_demos != 0){
            for (size_t i = 0; i < len; i++){
                d_subs.push_back(wax_demo(sub_demos-1, len));
            }
        }
    }
    ~wax_demo(void){
        /* NOP */
    }
    void get(const wax::type &key, wax::type &value){
        if (d_subs.size() == 0){
            value = d_nums[wax::cast<size_t>(key)];
        }else{
            value = obj::cast(&d_subs[wax::cast<size_t>(key)]);
        }
    }
    void set(const wax::type &key, const wax::type &value){
        if (d_subs.size() == 0){
            d_nums[wax::cast<size_t>(key)] = wax::cast<float>(value);
        }else{
            throw std::runtime_error("cant set to a wax demo with sub demos");
        }
    }
};

#define transform(i, j, k) float(i * j * k + i + j + k);

int main(void){
    try{
        wax_demo wd(2, 10);
        //test chained access
        std::cout << "chain 1" << std::endl;
        wd[size_t(0)];
        std::cout << "chain 2" << std::endl;
        wd[size_t(0)][size_t(0)];
        std::cout << "chain 3" << std::endl;
        wd[size_t(0)][size_t(0)][size_t(0)];
        //set a bunch of values
        std::cout << "set and get all" << std::endl;
        for (size_t i = 0; i < 10; i++){
            for (size_t j = 0; j < 10; j++){
                for (size_t k = 0; k < 10; k++){
                    float val = transform(i, j, k);
                    //std::cout << i << " " << j << " " << k << std::endl;
                    wd[i][j][k] = val;
                    BOOST_ASSERT(wax::cast<float>(wd[i][j][k]) == val);
                }
            }
        }
        //test storing a proxy
        std::cout << "store proxy" << std::endl;
        wax::proxy p = wd[size_t(0)][size_t(0)];
        p[size_t(0)] = float(5);
        //test printing a type
        std::cout << "print type" << std::endl;
        wax::type test_type = float(3.33);
        std::cout << test_type << std::endl;
        std::cout << "done" << std::endl;
    }catch(std::exception const& e){
        std::cout << "Exception: " << e.what() << std::endl;
        return ~0;
    }
    return 0;
}
