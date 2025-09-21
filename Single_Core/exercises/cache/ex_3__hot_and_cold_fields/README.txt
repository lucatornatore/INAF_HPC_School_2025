
Here we want to explore	how the	hot/cold splitting helps when it reduces compulsory
cache line fetches for the hot path, or fits more hot items per line for a better
spatial locality.
However, if we are dominated by latency, as in the pointer_chasing case,
this effect is not detectable.
