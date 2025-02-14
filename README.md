# Hash Table Experiments

Inspired by https://arxiv.org/abs/2111.12800

## Data

```
Open Addressing Table
Min/Max/Median/Average time: 122507/217033/140647/146901us

Open Addressing 2x Table
Min/Max/Median/Average time: 88831/136483/96425/97355us

Tiny Storage
Min/Max/Median/Average time: 65607/122136/67239/75566us
```

multi-level hash table shows a way better performance comparing to a lower load factor hash table with the same memory cost.


## Why?

Assume we have a hash table with a load factor of $d$ ($0 < d < 1$) and a memory cost of $M$.

If we want to insert a new key, the possible of collision is $d$.
Assume we have a perfect hash function with uniformly distribution,
The average time to insert a new key is $\frac{1}{1-d}$.

By doubling the memory cost, we reduce the load factor to $d/2$,
The average time to insert a new key is $\frac{1}{1-d/2}$.

So, the speed ratio is $\frac{d - 2}{2d - 2}$.

With the same memory cost, we can introduce a multi-level hash table,
which means that we have a bucket array, and the bucket array itself is a hash table.
Assume the bucket size has the same size of the total keys.

With the load factor of $d$, The key only has $d^2$ possibility to hit the same bucket.
By hitting the same bucket, we push the key to the overflow bucket array using open addressing hash table.
Thus, $d^2$ keys will be pushed to the overflow array, and the load factor of the overflow bucket array is $(1-d)^2$.
And, $(1-d^2)$ keys will be pushed to the bucket array, and the load factor of the bucket array is $(1-(1-d)^2)$.

By inserting a new key, the time cost is $(d^2) \times (\frac{1}{1 - (1-d)^2})$, otherwise, it is $O(1)$.
Thus the cost is $\frac{d}{2-d}$.

The speed ratio is $\frac{d-2}{d^2 - d}$.

When d = 0.586, the speed up ratio is 5.8x, which is the worst case if the constant O(1) is treated negligible.

In real world application, the constant O(1) is not negligible, so the speed up ratio is worse.
We then introduce a bucket to have multiple keys as a linked list, and the bucket array itself has fewer keys.
By experiment, if the size of the bucket array is 1/4 of the total keys,
and the bucket size is 2 for better cache performance (MAGIC NUMBER),
we can get the speed up ratio at ~2.9x,
which is better comparing to have a lower load factor hash table with the same memory cost.
