__version__ = '0.16.2+cu121'
git_version = 'c6f39778e636ec40a69bdbc74386818c57a65af3'
from torchvision.extension import _check_cuda_version
if _check_cuda_version() > 0:
    cuda = _check_cuda_version()
