# Fixed-Income Quantitative Simulation Engine (C++)

본 프로젝트는 이자율 기간구조 모델링과 마르코프 연쇄(Markov Chain) 기반의 신용등급 전이를 결합한 **채권 포트폴리오 백테스팅 및 관리 시뮬레이션 엔진**입니다. 금융공학적 프라이싱 원리를 바탕으로 시장의 동적 변화를 모사하며, 거래 비용 및 세무 로직을 포함하여 실무에 가까운 투자 환경을 제공합니다.

---

## 1. 핵심 금융 모델링 (Core Modeling)

### 1.1 이자율 기간구조: CIR (Cox-Ingersoll-Ross) 모델
무위험 금리의 동학을 모델링하기 위해 제곱근 확산 프로세스를 따르는 **CIR 모델**을 사용합니다. 이 모델은 금리가 항상 양수($ > 0 $)를 유지하며 장기 평균으로 회귀하는 성질을 가집니다.

$$dr_t = \kappa(\theta - r_t)dt + \sigma_r \sqrt{r_t} dW_t$$

* **$\kappa$ (Kappa):** 평균 회귀 속도 (가령 0.15)
* **$\theta$ (Theta):** 장기 평균 금리 수준 (가령 0.035)
* **$\sigma_r$ (Sigma):** 금리 변동성 (가령 0.012)
* **$dW_t$:** 위너 프로세스 (Standard Brownian Motion)

평가 시점의 만기별 할인율은 아래의 채권 가격 결정식을 통해 연속 복리 방식으로 산출됩니다.

$$P(t, T) = A(t, T) e^{-B(t, T) r_t}$$



### 1.2 신용 위험: 마르코프 전이 행렬 (Credit Migration)
채권의 신용 등급은 고정되지 않고 매 분기 확률적으로 변동합니다. 본 엔진은 실제 신용평가사의 데이터를 벤치마킹하여 투자적격등급과 투기등급 간의 전이를 다음과 같이 모델링하였습니다.

| From \ To | AAA | AA | BBB | BB | B | C | Default |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **AAA** | 0.97 | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| **AA** | 0.02 | 0.93 | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |
| **BBB** | 0.00 | 0.04 | 0.88 | 0.06 | 0.01 | 0.00 | 0.01 |
| **BB** | 0.00 | 0.00 | 0.05 | 0.82 | 0.08 | 0.02 | 0.03 |
| **B** | 0.00 | 0.00 | 0.00 | 0.05 | 0.77 | 0.10 | 0.08 |
| **C** | 0.00 | 0.00 | 0.00 | 0.00 | 0.05 | 0.65 | 0.30 |



---

## 2. 채권 프라이싱 및 유동성 (Pricing & Liquidity)

### 2.1 수익률(YTM) 구성
각 채권의 최종 수익률은 시장 금리에 개별 리스크 프리미엄을 가산하여 결정됩니다.

$$YTM = R(t, T) + Spread_{base} + Spread_{shock} + Liquidity_{penalty}$$

* **$R(t, T)$:** CIR 모델 기반 무위험 금리 (Spot rate)
* **$Spread_{base}$:** 등급별 기본 신용 스프레드 (AAA: 60bp, BBB: 350bp, C: 1800bp 등)
* **$Spread_{shock}$:** 시장 전체의 신용 경색 정도를 나타내는 확률적 충격 (신용 사이클 반영)
* **$Liquidity_{penalty}$:** Off-the-run(기발행) 채권에 부여되는 유동성 프리미엄 (25bp)

### 2.2 호가 스프레드 (Bid-Ask Spread)
현실적인 체결을 위해 등급별 유동성 비용을 차등 적용합니다.
* **Investment Grade (BBB 이상):** 5bp ~ 25bp
* **Junk Bond (BB 이하):** 50bp ~ 300bp
* **Off-the-run:** 유동성 부족으로 인해 위 스프레드의 **2배** 적용

---

## 3. 포트폴리오 비용 인식 (Portfolio Accounting)

* **원천징수 로직:** 모든 쿠폰 이자는 대한민국 이자소득세율인 **15.4%**를 적용하여 세후 금액으로 현금 계정에 산입됩니다.
* **거래 비용(Slippage) 추적:** 매수 시 $Ask - Mid$, 매도 시 $Mid - Bid$ 차액을 거래 비용으로 누적 집계하여 포트폴리오의 매매 효율성을 측정합니다.
* **부도 처리:** 부도 발생 시 원금의 **40%(Recovery Rate)**만 회수하며, 나머지 원금은 전액 손실 처리됩니다.

---

## 4. 시스템 구조 (Architecture)

프로젝트는 유지보수가 용이하도록 모듈형으로 설계되었습니다.

* `bond.h`: CashFlow 및 Bond 구조체 정의
* `market.h / .cpp`: CIR 모델 기반 가격 결정 및 신용 전이 엔진
* `portfolio.h / .cpp`: 자산 계정 및 거래 로그 관리
* `utils.h / .cpp`: ASCII 기반 시각화 및 CLI 유틸리티
* `main.cpp`: 시뮬레이션 루프 및 사용자 인터페이스

---

## 5. 사용 가이드 (Usage)

터미널에서 다중 명령(`[,]` 구분)을 지원합니다.

* **gy**: 현재 시점의 수익률 곡선(Yield Curve) 출력
* **ls [category]**: 종목 필터링 (`gov`, `ig`, `junk` 또는 특정 등급 검색)
* **gv / gd / gc**: 자산 가치, 듀레이션, 볼록성의 시계열 추이 그래프 확인
* **b / s [ticker] [amount]**: 채권 매수 및 매도 (거래 비용 자동 산출)
* **n / y**: 1분기 또는 1년의 시간 경과 진행 (쿠폰 수취 및 등급 변동 발생)

---

## 6. 예시

<img src="https://github.com/user-attachments/assets/59f61f12-2f48-47b2-b1af-39f03ceabdaf" width="600" alt="S1">

<img src="https://github.com/user-attachments/assets/fceaaeb7-96b1-41af-9da4-7bdfe08050cc" width="600" alt="S2(4.25y)">

<img src="https://github.com/user-attachments/assets/156b6c40-0f63-4318-b007-c29697f34ec1" width="600" alt="IG">

<img src="https://github.com/user-attachments/assets/17cc9db1-4e1d-4b0b-b602-40623f3bfa00" width="600" alt="AA">


<img src="https://github.com/user-attachments/assets/aff12e10-34ec-42d0-a498-aedc5b751f60" width="600" alt="Term structure">

<img src="https://github.com/user-attachments/assets/b023c413-413e-49a4-88ba-b7f1006940a7" width="600" alt="Portfolio Value">

<img src="https://github.com/user-attachments/assets/a2041888-5024-4d44-93dd-7a39d5cc42c4" width="600" alt="Duration">

<img src="https://github.com/user-attachments/assets/d5d5a0d6-6d40-4ad5-b8dd-05481176b2e6" width="600" alt="Convexity">
---

## 7. 빌드 및 실행 (Build)

C++11 이상의 컴파일러가 필요합니다.

```bash
# 컴파일
g++ main.cpp market.cpp portfolio.cpp utils.cpp -o bond_sim

# 실행
./bond_sim

---




