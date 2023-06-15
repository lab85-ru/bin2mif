# bin2mif
Convert BIN file to Altera MIF & Xilinx COE & ModelSim MEM & GoWin MI & Lattice MEM.

Altera/Intel:
bin2mif -mif -datawidth X -i file-input-bin -o file-output.MIF

Xilinx/AMD:
bin2mif -coe -datawidth X -i file-input-bin -o file-output.COE

ModelSim:
bin2mif -mem -datawidth X -i file-input-bin -o file-output.MEM

GoWin:
bin2mif -mi -datawidth X -i file-input-bin -o file-output.MI

Lattice:
bin2mif -lattice_mem -datawidth X -i file-input-bin -o file-output.MEM
