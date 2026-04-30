#!/bin/bash
#SBATCH -J 414
#SBATCH --account=proj13
#SBATCH --nodes=1
#SBATCH --nodelist=kolyoz[25-30,32-60]
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --partition=kolyoz-cuda
#SBATCH --gres=gpu:1
#SBATCH --time=0-12:00:00
#SBATCH --output=res/414-%j.out
#SBATCH --error=res/414-%j.err
#SBATCH --export=NONE

hostname

repo_directory="/arf/home/delbek/414/"

module purge
unset SLURM_EXPORT_ENV

source /etc/profile.d/modules.sh

module use /arf/sw/modulefiles

module load comp/cmake/3.31.1
module load comp/gcc/12.3.0
module load lib/cuda/13.0
module load lib/openmpi/4.1.6
module load comp/python/3.12.0

export SRUN_CPUS_PER_TASK=${SLURM_CPUS_PER_TASK:-16}
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK:-16}

mkdir -p ${repo_directory}build
cd ${repo_directory}build
cmake ..
make
cd ..

./build/414
