    {$list.new a b c {d e}}

*list.new* creates a new list containing its arguments.

    {$list.len {a b c}}

*list.len* returns the length of a list.

    {$list.append $list f}

*list.copy* copies a list.

    {$list.copy $list}

*list.append* appends an item to a list.

    {$list.extend $list {f g}}

*list.extend* adds all items of a list to an existing list.

    {$list.index {a b c} 0}

*list.index* returns the list item at a given index.

    {$list.range {a b c d e f} 2 4}

*list.range* returns the items of a list, between two indices.

    {$list.in a {a b c}}

*list.in* determines whether an item belongs to a list.

    {$list.pop $list 1}

*list.pop* removes the item at a given index from a list.

    {$choice {a b c}}

*choice* returns a pseudo-random list item.
