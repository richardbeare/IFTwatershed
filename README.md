IFTwatershed
============

Code for implementation of image foresting transform style watershed in ITK.

The IFT papers appear to be based on a flawed understanding of how
a traditional watershed actually works - namely the idea that a pair
of markers hitting sides of a basin with equal heights and no marker of
its own will result in that basin being grabbed by one of the regions
rather than being split between them. This ignores the lower completion
step that happens implicitly in queue based implementations. It is thus
unclear how much of the IFT formulation is dealing with the flawed
implementation.

Focus of this repo is now the dissimilarity code, which allows gradients
to be computed internally.