![Supply and demand](etc/graph.png?raw=true "Supply and demand")

# doubleauction

A matching engine for double auctions.

This program takes buy and sell (limit) orders, finds the market-clearing
price and quantity and returns the remaining order book. This program is
currently able of finding the equilibrium of hundreds of millions of orders
in seconds.

Compile using

	make

## Usage and important notes

Run the program using `./doubleauction`. If you add a filename as an
argument, the order book will be read from that file. If you do not
supply an argument, the order book is read from stdin.

Each order is a line like this:

	B 100 15 71 5

The first field is either *B* or *S*, indicating whether it is a buy
or sell order. The second field is the bid/ask. The third field is the
corresponding quantity. The fourth field is the order ID: It is not
directly used, but if you use doubleauction in conjunction with another
program, this allows you to keep track of what happens with your order.
The fifth field is the priority of the order. Importantly, a lower
number here indicates *higher* priority. This means that if your order
has the same bid/ask as another, but your order has a higher priority,
your order will be fulfilled first. If you put in *0* as the priority,
we set the priority to nanoseconds since Unix epoch, earlier arrival
still implying preferred fulfillment.

The type of the second and third field are determined by you in the
source code (currently `uint64_t` and `int64_t`, respectively). Please
note the rampant difficulties that arise when using floating-point
arithmetic, and consider avoiding the corresponding types. Also note
that for computational reasons, quantites may become negative, while
prices are always nonnegative. The type of the fourth and fifth field
are always `uint64_t`.

After reading the order book, `doubleauction` will attempt to find a
market equilibrium. If one is found, the market sides transact accordingly
and fulfilled orders are deleted or altered (if there is only partial
fulfillment). Thereafter, the resulting order book is output to stdout
and some other information is output to stderr. (Thus, theoretically, you
can pipe several instances of `doubleauction` together -- and if my code
is correct, no equilibrium should ever be found after the first stage.)

This program asserts optimality (i.e. there is no higher feasible quantity)
and feasibility (i.e. individual rationality when transacting at some price)
and aborts if they are not met. Still, there is no guarantee and use of
this program is entirely without warranty.

## Example markets

Some example order books are supplied in the directory `markets/`.

For example, the market in the above graphic is available in
`markets/example.mkt`. Running `./doubleauction markets/example.mkt`
gives the equilibrium. Visual inspection confirms that the
market-clearing price is between *p* = 58 and *p* = 59, with quantity *q* = 697.
Any higher quantity leads to a contradiction (not individually rational/feasible).
If there is a range of feasible prices, we use split-the-difference.
(Note, however, that the effectiveness of split-the-difference may be
constrained by integral types. Thus, I suggest you run this example
both with integral and floating-point types; see lines 37, 38 in `main.cpp`.)

There is also an example that does not have an equilibrium, `markets/noeq.mkt`.

There is also a script to generate markets, `etc/genmkt.php` and a
script to plot curves like the above, `etc/graph.R`. Sadly, the latter
still contains some bugs that will likely not be fixed.

doubleauction has been tested with up to 200 million orders (approximately
half of which were buy orders). The correct equilibrium was found after
119 seconds.

## Licensing

This software is licensed under the GNU Affero General Public License v3.0.
Any acknowledgment of use in academic research is appreciated but not
technically required.

## ToDo

- sort order book in place
- market orders
- immediate or cancel orders
- proper messaging (to keep track of what happened to every order)
- order deletion
- parallelization?
- more testing
