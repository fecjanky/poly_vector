# poly_vector definitions & invariants

|Symbol     | definition
|-----------|---------------------|
|Ai         |alignment of ith elem
|S'i 		|size of ith elem
|Amax 		|max(Ai) over every i
|Si 		|((S'i+Amax-1)/Amax)*Amax 
|Ri 		|max(Sj) for j = i+1 .... N
|Amax(Ti)	|Max alignment at time of insertion or reallocation

#### Invariant #1
every element i which is inserted to the vector is stored aligned to Amax(Ti) with storage occupancy of integral multiple of Amax(Ti) until:
- removal of element i
- reallocation happens

#### Invariant #2
Amax can change only in case of reallocation  or when the vector is empty but in that case it can increase or decrease as well 

If all above are kept than maximal storage requirement of a vector for its object is 
B := Amax-1 + sum(Si)

#### Exception safety requirements for insertion:
- single element at end  or elements is either copyable or no-throw moveable => strong guarantee 
- else => basic guarantee

#### Exception safety requirements for removal
- If the removed elements include the last element in the container => no-throw guarantee
- else => (basic guarantee).

## Reallocation procedure
- Calculate Amax
- Calculate B
- Allocate B bytes of memory
- Construct element with respect to invariant no. 1

## Removal procedure
- Destruct elements
- F = freed storage
- Calculate Ri for every subsequent elems
- Move/copy backward objects until F - sum(Si) > Rj with j > i holds
- Move all pointers backward
