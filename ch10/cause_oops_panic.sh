sudo sh -c "echo 1 > /proc/sys/kernel/panic_on_oops"
sudo sh -c "echo 1 > /proc/sys/kernel/sysrq"
sync; sleep .5
sudo sh -c "echo c > /proc/sysrq-trigger"
