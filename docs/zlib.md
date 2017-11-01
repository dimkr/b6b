    {$deflate [[$open file.txt ru] read] 9}

*deflate* compresses a string.

    {$inflate [$deflate [[$open file.txt ru] read]]}

*inflate* decompresses a string compressed using *deflate*.
