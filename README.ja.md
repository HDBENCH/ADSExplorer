# ADS Explorer — NTFS 代替データストリーム(ADS)可視化シェル拡張

[English](README.md) | **[日本語](README.ja.md)**

Windows エクスプローラー上で **NTFS の代替データストリーム (Alternate Data Streams, ADS)** を可視化する C++ 製シェル拡張です。

- 🏷️ **アイコンオーバーレイ** — ADS を持つファイルに紫のドクロアイコンを重ねて表示
- 🗂️ **プロパティのタブ** — ファイル／フォルダのプロパティの **「ADS」タブ**で、ストリーム名・サイズ・内容を一覧。**選んだストリームの削除**もできます
- 🌐 **日本語 / 英語 自動切り替え** — Windows の表示言語に応じて UI を自動でローカライズ
- 📦 **MSI インストーラー同梱** — 英語 / 日本語 UI を 1 本に内包（レジストリ登録は MSI が宣言的に管理）。再起動したくない場合向けに regsvr32 ベースの手動インストール版 zip も同梱

ATL/WRL や .NET に依存しない素の Win32/COM 実装で、**静的 CRT リンク**のため VC ランタイム未導入の PC でも動作します。

---

## 動作環境

| 項目 | 内容 |
|------|------|
| OS | Windows 10 / 11 (x64) |
| ファイルシステム | NTFS (ADS は NTFS の機能。FAT/exFAT では ADS 自体が存在しません) |
| ビルド | Visual Studio 18 (v145 ツールセット) + Windows SDK 10.0.26100 |

---

## クイックスタート

### 1. ビルド

**A. Visual Studio で開く場合**

1. `ADSExplorer.sln` を Visual Studio で開く
2. 構成を **Release / x64** に設定
3. ビルド（`Ctrl+Shift+B`）
4. `build\x64\Release\ADSExplorer.dll` が生成されます

**B. コマンドラインの場合**（開発者コマンドプロンプト）

```bat
"%ProgramFiles%\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
msbuild src\ADSExplorer.vcxproj -p:Configuration=Release -p:Platform=x64
```

### 2. MSI インストーラーを作る

初回のみ [WiX Toolset v6](https://wixtoolset.org/) を .NET ツールとして導入します:

```bat
dotnet tool install --global wix --version 6.*
wix extension add -g WixToolset.UI.wixext/6.0.2
```

導入後は **Visual Studio で Release / x64 をビルドするだけで** `dist\ADSExplorer-<バージョン>.msi` まで自動生成されます（`ADSExplorer.vcxproj` の `BuildMsiPackage` ターゲット）。

**バージョン番号は [`src/ADSExplorer.rc`](src/ADSExplorer.rc) の `PRODUCTVERSION` から取得されます。** リソースが唯一の情報源なので、そこを書き換えれば DLL・MSI のファイル名・MSI の `ProductVersion`・zip の名前がすべて追随します。Windows Installer は `ProductVersion` の先頭 3 フィールドしか比較しないため、4 つ目は捨てられます。`PRODUCTVERSION` と同ファイル内の `ProductVersion` 文字列が食い違う場合は `tools\build-msi.ps1` が警告します。

| 条件 | 挙動 |
|------|------|
| Release / x64 | DLL ビルド後に MSI を生成 |
| Debug | MSI は生成しない |
| WiX 未導入 | 警告のみ。DLL のビルドは成功する |
| MSI を作りたくないとき | `msbuild /p:BuildMsi=false`（または vcxproj の `BuildMsi` 既定値を `false` に） |
| バージョンを上書きしたいとき | `msbuild /p:MsiVersion=1.2.3`（通常は不要） |

DLL・`.wxs`・`.wxl`・説明書のいずれも変更がなければ MSI 生成はスキップされるため、通常の増分ビルドは遅くなりません。

MSI だけを作り直す場合:

```bat
powershell -ExecutionPolicy Bypass -File tools\build-msi.ps1
```

生成される MSI は **1 本で英語 UI と日本語 UI の両方を内包**します（日本語版は言語トランスフォーム `1041` として MSI に埋め込まれ、Windows のユーザーロケールに応じて自動選択されます）。言語を明示するには:

```bat
msiexec /i ADSExplorer-<バージョン>.msi TRANSFORMS=":1041"   :: 日本語 UI を強制
```

### 3. インストール

`ADSExplorer-<バージョン>.msi` をダブルクリックし、ウィザードに従います（管理者への昇格を求められます）。無人インストールは `msiexec /i ADSExplorer-<バージョン>.msi /qn`。

DLL は `%ProgramFiles%\ADS Explorer\` に配置され、シェル拡張のレジストリ登録は **MSI が宣言的に行います**（`regsvr32` は不要）。

**インストール／アンインストールの最後に Windows の再起動が必要です。** `ADSExplorer.dll` はエクスプローラーにロードされるため、稼働中は上書きも削除もできません。インストーラーが「エクスプローラーを閉じてください」と要求するのは一般ユーザーには対処困難なので、次の方針にしています:

- `MSIRESTARTMANAGERCONTROL=Disable` — Restart Manager によるエクスプローラー停止を試みない
- 「使用中のファイル」ダイアログ（`FilesInUse` / `MsiRMFilesInUse`）を MSI から削除 — ビルド時に `tools\build-msi.ps1` が除去する
- `REBOOT=Force` — 最後に必ず再起動を要求する

ロックされたファイルは Windows Installer が「再起動時に置換／削除」へ回します。オーバーレイハンドラーはエクスプローラー起動時にしか読み込まれないため、どのみち再起動は必要です。無人配布などで抑止したい場合は `msiexec ... REBOOT=ReallySuppress` を渡せます。

### 4. 配布パッケージを作る

```bash
bash tools/package.sh
```

`dist\` の中身:

| ファイル | 内容 |
|----------|------|
| `ADSExplorer-<バージョン>.msi` | MSI インストーラー（英語 / 日本語 UI を内包）|
| `ADSExplorer-<バージョン>-manual.zip` | 手動インストール版（`ADSExplorer.dll` + `install.bat` + `uninstall.bat` + 専用説明書）|
| `readme_jp.txt` / `README.txt` | MSI 版の説明書 |

MSI・zip ともファイル名にバージョンが入るため、別バージョンをビルドしても上書きされず並んで残ります。説明書はすべて CRLF、日本語のものは BOM 付き UTF-8 に整形済みです。組み立ては `tools\build-msi.ps1` に集約してあるので、Visual Studio の Release ビルドでも `bash tools/package.sh` でも `dist\` の内容は同じになります。

### 手動インストール版について

`regsvr32` で直接登録する従来方式を zip にまとめたものです。MSI 版との違い:

| | MSI 版 | 手動版 |
|---|---|---|
| Windows の再起動 | 必要 | 不要（エクスプローラーのみ再起動）|
| 「インストールされているアプリ」への表示 | あり | なし |
| DLL の場所 | `%ProgramFiles%\ADS Explorer\` | 展開したフォルダのまま |
| ロールバック / 修復 | Windows Installer が管理 | なし |

再起動できない環境や、USB メモリなどから一時的に使いたい場合の代替手段です。**両方を同時に登録しないでください** — 別々のパスを指す登録が 2 つでき、片方を消してももう片方が残ります。`install.bat` は MSI 版の存在を検出して警告します。

---

## 使い方

1. インストール後、エクスプローラーで ADS を持つファイルを見ると、アイコンにドクロのバッジが重なります。
2. ファイル（またはフォルダ）を右クリックし **「プロパティ」** を選び、**「ADS」タブ** を開きます。
   - **ファイル / ストリーム名 / サイズ / 内容** の 4 列を表示します。ファイル 1 つのときは「ファイル」列を省略します。
   - 一覧からストリームを選び **「選択した ADS を削除」** または **Delete キー** で削除できます（確認あり・**元に戻せません**）。ファイル本体は残り、選んだ名前付きストリームだけが消えます。
   - **Ctrl / Shift クリックで複数選択**、**Ctrl+A で全選択**して一括削除できます。一部が失敗しても残りの削除は続行し、最後に失敗した分をまとめて報告します。
   - 削除は（確認のうえ）その場で実行されます。OK／適用ボタンを待ちません。
   - 複数ファイルを選択してからプロパティを開くと、まとめて一覧できます。
   - FAT / exFAT などストリームを持てないドライブ上では、タブ自体を追加しません。

> UI（タブ・一覧・確認メッセージ）は、Windows の表示言語が日本語なら日本語、それ以外は英語で自動表示されます。

### ADS を試しに作る / 確認する

コマンドプロンプトで:

```bat
rem 通常ファイルを作成
echo こんにちは> sample.txt

rem 代替データストリーム "secret" を追加
echo 隠しデータ> sample.txt:secret

rem ADS を確認 ( /r オプションでストリーム表示 )
dir /r
```

`sample.txt:$DATA`（本体）に加えて `sample.txt:secret:$DATA` が表示されれば ADS が付いています。
インターネットからダウンロードしたファイルには、Windows が自動で `Zone.Identifier` という ADS を付与します（ゾーン情報＝MOTW）。これも本拡張で可視化・削除できます（`Zone.Identifier` の削除は「ブロックの解除」に相当します）。

---

## アンインストール

「設定」→「アプリ」→「インストールされているアプリ」から **ADS Explorer** をアンインストールしてください。コマンドラインからは `msiexec /x ADSExplorer-<バージョン>.msi /qn`。

ファイル・レジストリ登録はすべて Windows Installer の管理下にあるため、痕跡を残さず削除されます。

---

## 仕組み（技術詳細）

2 つの COM シェル拡張を 1 つの DLL に実装しています。

| 機能 | 実装インターフェース | 登録先 |
|------|----------------------|--------|
| アイコンオーバーレイ | `IShellIconOverlayIdentifier` | `HKLM\...\Explorer\ShellIconOverlayIdentifiers` |
| プロパティのタブ | `IShellExtInit` + `IShellPropSheetExt` | `HKCR\*\shellex\PropertySheetHandlers`、`HKCR\Directory\shellex\...` |

- ADS の検出には `FindFirstStreamW` / `FindNextStreamW` を使用し、既定ストリーム `::$DATA` 以外が存在すれば「ADS あり」と判定します。
- ADS の削除は `DeleteFileW("ファイルパス:ストリーム名")` で行い、名前付きストリームだけを消去します（本体ファイルは残ります）。
- オーバーレイの `IsMemberOf` はエクスプローラーが表示する全ファイルに対して呼ばれるため、`FILE_ATTRIBUTE_OFFLINE` やクラウドのプレースホルダ（`RECALL_ON_OPEN` 等）はスキップし、不要な実体化・ダウンロードを避けています。
- UI の文字列は `GetUserDefaultUILanguage()` で表示言語を判定し、日本語/英語を自動で切り替えます（`Lang.cpp`）。
- オーバーレイアイコンは DLL に埋め込んだ `overlay.ico`（`src\res\overlay.ico`）を使用します。再生成する場合は `bash tools/genicon.sh` を実行してください。

---

## ⚠️ 既知の制約と注意

### 1. オーバーレイアイコンは「システム全体で 15 個まで」

これは Windows の仕様です。オーバーレイハンドラーはアルファベット順で先頭 15 個までしか読み込まれません。本拡張は登録名を先頭空白付き（`  ADSExplorer`）にしてソート順を早めていますが、**すでに 15 枠が埋まっているとバッジが表示されません**。

> **よくある競合相手**: `OneDrive`、`Dropbox`、`TortoiseSVN` / `TortoiseGit` など。とくに Tortoise 系はそれぞれ 7 個前後のオーバーレイを登録するため、入っていると枠がほぼ埋まっていることがあります。

確認方法（レジストリエディタ）:
```
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers
```
ここのサブキー数が 15 を超えていると、末尾のものは無視されます。使わないオーバーレイ（例: 未使用のクラウド同期）を削除すると本拡張のバッジが出るようになります。
なお **プロパティの「ADS」タブはこの制限と無関係**に常に利用できます。

### 2. エクスプローラーのカラム（列）には表示できません

任意のファイルにカスタム列を追加する旧 API `IColumnProvider` は **Windows Vista で廃止**され、Windows 10/11 では機能しません。現行の代替（プロパティハンドラー）は拡張子単位の登録が前提で「全ファイルに ADS 列」は技術的に不可能です。本プロジェクトではこの制約を踏まえ、**プロパティの「ADS」タブ**でストリーム名・サイズ・内容を表示する方式を採用しています。

### 3. パフォーマンス

`IsMemberOf` は表示中の全ファイルに対して呼ばれます。非常に多数のファイルを含むフォルダでは、ADS 判定のオーバーヘッドが体感される可能性があります。

### 4. 権限

インストールには管理者権限が必要です（`HKLM` への書き込みを伴うため）。MSI は per-machine インストールなので、実行時に UAC の昇格が求められます。

なお DLL 内の `DllRegisterServer` / `DllUnregisterServer`（`src\Reg.cpp`）は開発時のデバッグ用に残してありますが、MSI 経由のインストールでは使われません。両者を変更するときは内容を一致させてください。

---

## プロジェクト構成

```
ADS/
├── ADSExplorer.sln              ソリューション
├── README.md                    (English)
├── README.ja.md                 (日本語)
├── LICENSE                      MIT License
├── .gitignore
├── .gitattributes               改行コードの固定 (.bat=CRLF / .sh=LF)
├── src/
│   ├── ADSExplorer.vcxproj      プロジェクト (v145 / x64 / DynamicLibrary / 静的CRT)
│   ├── ADSExplorer.def          エクスポート定義
│   ├── ADSExplorer.rc           リソース (ダイアログ / アイコン / バージョン)
│   ├── resource.h
│   ├── DllShared.h              共有グローバル
│   ├── Guids.h / Guids.cpp      CLSID 定義
│   ├── AdsUtil.h / .cpp         ADS 検出・列挙・プレビュー
│   ├── ClassFactory.h / .cpp    IClassFactory
│   ├── IconOverlay.h / .cpp     IShellIconOverlayIdentifier
│   ├── PropSheet.h / .cpp       IShellExtInit + IShellPropSheetExt (「ADS」タブ)
│   ├── Selection.h / .cpp       シェルのデータオブジェクトから選択パスを取得
│   ├── StreamList.h / .cpp      ADS 一覧・削除 (タブが表示)
│   ├── Lang.h / Lang.cpp        UI 言語判定 (日本語 / 英語)
│   ├── Reg.h / .cpp             登録 / 解除
│   ├── dllmain.cpp              DllMain と COM エントリポイント
│   └── res/overlay.ico          オーバーレイアイコン (紫のドクロ)
├── tools/
│   ├── genicon.sh               overlay.ico 生成スクリプト
│   ├── build-msi.ps1            MSI ビルド (WiX v6 + 言語トランスフォーム埋込)
│   └── package.sh               dist/ 配布フォルダ生成スクリプト
├── installer/
│   ├── wix/
│   │   ├── ADSExplorer.wxs      MSI 定義 (ファイル + レジストリ + UI)
│   │   ├── en-us.wxl            英語リソース
│   │   ├── ja-jp.wxl            日本語リソース
│   │   ├── License.en.txt       ライセンス原文 (ビルド時に RTF 化)
│   │   └── License.ja.txt
│   ├── manual/                  手動インストール版 zip 用の説明書
│   │   ├── readme_jp.txt        (日本語)
│   │   └── readme_en.txt        (English)
│   ├── install.bat              手動登録 (regsvr32 + Explorer 再起動)
│   ├── uninstall.bat            手動解除
│   ├── readme_jp.txt            MSI 版 (日本語)
│   └── readme_en.txt            MSI 版 (English)
└── dist/                        配布用 (ビルド時に生成 / git 管理外)
    ├── ADSExplorer-<バージョン>.msi
    ├── ADSExplorer-<バージョン>-manual.zip
    ├── readme_jp.txt
    └── README.txt
```

---

## ライセンス

MIT License / (C) 2026 HDBENCH.NET
