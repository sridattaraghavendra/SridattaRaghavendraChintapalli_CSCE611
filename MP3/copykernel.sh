# Replace "/mnt/floppy" with the whatever directory is appropriate.
sudo mount -o loop mp3.img /mnt/floppy
sudo cp kernel.bin /mnt/floppy
sleep 1s
sudo umount /mnt/floppy
