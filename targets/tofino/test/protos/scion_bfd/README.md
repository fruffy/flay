# A note to reproduce

```shell
# Setup veth
$SDE/install/bin/veth_setup.sh

cd $HOME/scion-p4/tofino-scion-br
# Tofino Model
$SDE/./run_tofino_model.sh --arch tofino2 -p scion -f evaluation/ports.json -c build/scion/scion/tofino2/scion.conf --int-port-loop 0xf
# switchd
$SDE/./run_switchd.sh -p scion --arch tofino2 -c p4src/with_cmac_1pipe.conf

# load config
## without bfd setup
$SDE_INSTALL/bin/python3.10 controller/load_config.py $HOME/scion-p4/tofino-scion-br/test_traffic/config/switch_config_2_sim.json --program_name scion

# with bfd setup
PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python sudo -E $SDE/install/bin/python3.10 controller/controller.py -k $HOME/scion-p4/tofino-scion-br/test_traffic/master0.key -c $HOME/scion-p4/tofino-scion-br/test_traffic/config/switch_config_2_sim.json -i veth251
```