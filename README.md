# kisti-dcgm
KISTI GPU profiling project
| 항목     | 버전     |
| ------ | ------ |
| Python | 3.10   |
| Conda  | 26.0.1 |
| Pip    | 26.0.1 |


| 항목   | 버전                |
| ---- | ----------------- |
| CUDA | 12.2              |
| DCGM | 4.5.3             |
| GPU  | NVIDIA Tesla V100 |

1. 환경 설정 스크립트 실행 후 conda 가상환경으로 접속
    source ./script/setting.sh
2. 빌드
    cd ./util
    make
3. ./<생성된_실행파일>


📂 Directory Structure (optional)

.
├── script/
│   └── setting.sh
├── util/
│   └── Makefile
└── README.md