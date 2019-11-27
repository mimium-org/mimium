[English](./design_proposal.md)


# 全体の構造

```
ソースコード　⇆ 抽象構文木(AST) ⇆ エディタ(別プロセス)
                            |
                            → 評価器 →オーディオ、MIDI出力

```
# 構文

### すべての変数宣言に対して時間の位置を付随させられる

```
input::int 1@100[ms] //これは絶対位置100msに置かれるinputという名前の整数1(@は時間付きであることを明示的に表す)　ブラケットで時間単位の指定(配列とややこしいかな)　時間の単位も自分で定義可能にしたい

1@[100,200,300,250][ms] 2個以上の時間を付随させることもできる、使うときに基本的には早い順にソートされる？のが良さそう

なんなら
1@[1,2,3...][sample] とかで無限数列定義できたらサンプルの扱いも楽な気がする
```

#### 時間シフト演算子 <<< と >>>

`hoge::int@ = 1@100[ms] >>> 1[sec] //now hoge is 1@1.1[sec]` 

この辺、みんな何使うのか困ってるぽい

Chuck 100 +=> now;

Chronic @+ @-

### 関数は入力に付随した時間のタイミングで発動する

**全ての変数に時間がバインドできるので、関数にその数字を突っ込むとその時刻にスケジュールされて発動する** (バインドされてない場合発行されない)

変数に2個以上時間をバインドした場合、一回何かの関数で使われると一番頭の時刻だけが消費されて、その関数内で何か別の関数を呼び出すと次の時刻を呼び出せる

```
pitch_index::int = produce((x)->{x+x},5)//[2,4,6,8,10]
time_index::int = produce((x)->{x*x},5) // [@1,@4,@9,@16,@25]
end_index = time_index.map((t)->{ t |> 2[s]}) //[@3,@6,@11,@18,@27]
 
 
pitchevents = pitch_index.tbind((x,i)->{x@[time_index[i],end_index[i]]}) //[2@[1,3],4@[4,6],6@[9,11],8@[16,18],10@[25,27]]
 
function noteOn(input_pitch::int,instrument){//input_pitch is like 2@[1,3] and trigger at time 1
 	instrument.pitch = input_pitch
 	instrument.gain = 1.0
 	bind(noteOff(input_pitch,instrument)) // now input_pitch is 2@3,will triggerd at time 3
}
function noteOff(input_pitch::int,instrument){
   	instrument.gain = 0.0
}
 
 pitchevents.map((pitch)->{noteOn(pitch)}) // bind noteon to all pitchevents elements
```

### 関数の中の任意の変数呼び出しで、過去/未来の値を参照できる、


```
 
delay(input::int@) = input@+100::ms //過去の入力を出力する

future(input::int@) = input@+100::ms //未来の入力を出力する(フィルターとか作るときに有効)

 //VSTプラグインのように全体にオフセットディレイがかかる感じ（Chronicでもやってた） 
```

### また、関数は自身の過去の出力をselfキーワードで参照できる

```
combfilter(input) = input + 0.999*( self<<<1 ) // 出力で未来の参照>>>は流石にエラー

combfilter = input+0.999*(self >>> 1::sec ) // //絶対時間で遡ることもビルトイン機能で配列の補完が効けばできる??無理かも
 
```


つまり、関数としてオーディオのパイプラインみたいに扱えるんだけど、内部的にはクラスのように値を保持している（配列の長さとかはコンパイル時に確定しなければエラーになる//faustと同じ）

## 環境変数

オーディオドライバなどから渡される値は環境変数≒グローバル変数として渡される
イミュータブルなもの(例えばサンプルレート)とミュータブル(例えばdacのアウト)なもの両方がある

環境変数は区別するために\#で参照される

## 2つ目のreturn,future
一定時間後に別の値を返す2つ目のreturnをfutureを書くことで予約できる
返り値の型はreturnと一致している必要がある

これと環境変数を組み合わせることでオーディオを以下のようなパイプライン形式で書くことができるようになる

↓サイン波の例

```
fn ramp(SR::int, incl::double){
    return self[-1]+incl
    future ramp(incl@+SR) //type of return & future should be the same;
}
(#SR,1) : ramp : %(4410) : /(4410) : *(PI*2) : sin　-> tmp1
//コロンはパイプライン演算子、->は右向き代入 自動カリー化される

tmp1+tmp2 -> out //みたいにしたときにtmp1とtmp2が違う間隔で実行されてたらどうやって同期します？tmp1が更新されたタイミングで自動で変更を伝搬？？？？

//多分こう

fn trigger(input){
	return input
	future trigger(input@+#SR)
}

trigger(tmp1+tmp2) -> out //periodically triggered in constant rate regardless update rate of tmp1 and tmp2

ではダウンサンプルはどうするか

fn make_downsample(){
	count = 0
	fn downsample(input){
		count = (count+1)%256
		return (count==0)? : input : STOP
	}
	return downsample
}
ds = make_downsample()
input : downsample

こうすると関数が値を返す時と返さない時とがある
何かnullでもvoidでもない別の予約ワードが必要？（エラーではないが、何も返さない、代入した時何も値を更新しない）

```

## あんまり本質と関係ないけど実装したい機能

### Dialect機能(2019/11/21現在やめたほうがいいかもとおもっている)

予約語や関数名に対するエイリアスなんだけど、1回限りしかできない（エイリアスのエイリアスは不可）
パフォーマンスとかで短く書く用　
コンパイル時にマクロ的に変換するので実行速度に影響しない(なんでもマクロを許すと大変なことになりそうなのでこういう制約をつけたい)


### 標準的な配列に対して初めから補完機能がついてる 

```
hoge = [20,30,0.12]
```
みたいなときに

`hoge[0.3]`みたいな呼び出し方ができ、20と30の間の値が出る(補完アルゴリズムは自分で定義もできる)


## 疑問/わかってない点

オーディオドライバ周り本当にそんなにうまくいくのか

音声ファイル周りの扱い

Dialectとか配列補完とか、中間表現にするさらに手前のプリプロセッサみたいな物を用意したほうがいいのかもしれない

