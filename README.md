# 宿題の説明

## 性能（Time/ Utilization）

### best fit & free list bin(2の累乗でサイズ分け)
* Challenge #1
   * time: 14 => 35
   * utilization: 70 => 70
* Challenge #2
   * time: 17 => 11
   * utilization: 40 => 40
* Challenge #3
   * time: 162 => 32
   * utilization: 9 => 51
* Challenge #4
   * time: 51411 => 3389
   * utilization: 15 => 72
* Challenge #5
   * time: 45843 => 2235
   * utilization: 15 => 75

## 工夫したこと
1. first fit から　best fitへ変更
2. free list binを11個作り、2の累乗でサイズ分け

## 考察
### best fit & free list bin(2の累乗でサイズ分け)
* challenge3~5では、best fit によりメモリ効率が上がった
    * challenge1, 2ではbest fit も first fitもメモリ効率は変わらなかった
      * このことから、challenge1, 2のように同じサイズのメモリを要求、解放する場合は、空きリストにある空き領域が同じサイズになるのでないかと考える
* free list binのサイズ分けにより、全てのchallengeで実行時間が短縮された
    * 同じサイズのメモリを要求、解放するchallenge1,2では短縮率が小さく、要求、解放するメモリサイズに幅がある3~5では短縮率が高かった
      * 要求、解放するメモリサイズが同じだと、同じサイズの空き領域ばかりできるので、とbinを分けても同じbinに空き領域が入ってしまい効果が小さいと考えられる
    