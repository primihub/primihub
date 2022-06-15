from primihub.FL.feature_engineer.bloom_filter import BloomFilter

if __name__ == '__main__':
    bloom_a = BloomFilter(200)
    bloom_b = BloomFilter(200)
    for i in range(50):
        bloom_a.add(str(i))
    for i in range(25, 75):
        bloom_b.add(str(i))
    bloom_union = BloomFilter.union(bloom_a, bloom_b)

    assert "51" not in bloom_a, "51 is in bloom_a, false positive occurred"
    assert "24" not in bloom_b, "24 is in bloom_b, false positive occurred"
    assert "55" in bloom_union
    assert "25" in bloom_union
