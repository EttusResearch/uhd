#!/usr/bin/bash

TOP_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/../../top

for device_top in e31x e320 n3xx x300 x400; do
    curr_path=$(realpath ${TOP_DIR}/${device_top})
    echo "Updating image cores in $curr_path..."
    cd $curr_path
    for yml_f in *core.yml; do
        echo "Regenerating from $yml_f..."
        rfnoc_image_builder -y $yml_f -H -D -G -l warning $@
    done
done
