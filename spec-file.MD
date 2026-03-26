# 🚗 Automated Parking Management System  

## 📖 Description
This system uses **Automatic Number Plate Recognition (ANPR)** and **proximity sensors** to automate parking operations.

---

## ⚙️ Features

### 1. Vehicle Registration
- Captures license plate via **outward-facing camera**
- Records **timestamp at entry**

---

### 2. RFID-Based Payment
- Cost: **500 RWF/hour**
- Rule:  
  - ⏱️ Any partial hour is **rounded up**

---

### 3. Exit Authorization
- Only vehicles with **completed payments** can exit

---

### 4. Proximity Sensing
- Sensors detect vehicles at:
  - Entry gate
  - Exit gate

---

### 5. Automated Gate Operation

#### 🚪 Entry
- Triggered when:
  - Vehicle detected
  - Plate successfully registered

#### 🚪 Exit
- Triggered when:
  - Vehicle detected
  - Plate recognized
  - Payment verified

---

### 6. 🚨 Alert System
- Detects **manual gate tampering**
- Logs:
  - License plate
  - Timestamp
  - Gate location (entry/exit)
- Triggers alarm

---

### 7. 📊 Dashboard
Displays real-time:
- Vehicle entries
- Exits
- Payments
- Unauthorized exit attempts

---

## 🧪 Tasks

### 🔹 System Design
1. Create **DFD** for vehicle check-in subsystem  

---

### 🔹 Hardware & Detection
2. 3D model of automated gate *(done)*  
3. Use sensor to detect vehicles *(done)*  
4. Trigger outward-facing camera *(done)*  

---

### 🔹 Plate Recognition
5. Implement **OCR** for license plate *(done)*  
6. Store plate in database  

---

### 🔹 Entry Flow
7. Register plate in DB  
8. Open gate after successful registration *(done)*  

---

### 🔹 RFID Payment
9. Use PoS to top up RFID *(done)*  
10. Load balance for demo *(done)*  
11. Process payments based on parking duration  

---

### 🔹 Exit Flow
12. Detect vehicle at exit *(done)*  
13. Activate inward-facing camera *(done)*  
14. Implement **LPR** *(done)*  
15. Verify payment *(done)*  
16. Open gate if verified *(done)*  

---

### 🔹 Security Handling
17. Detect unauthorized exit attempts  
18. Trigger alarm (siren/buzzer)  
19. Log incident in database  

---

### 🔹 Dashboard & UI
20. Display logs (entry, exit, payments, alerts)  
21. Build simple web interface  
22. Fetch data from DB  
23. Highlight unauthorized exits (timestamp + plate)  

---

## 🧾 Instructions

### 🔧 Hardware
- Already completed ✅

---

### 💻 Software Requirements

#### Languages
- Python  
- C / C++  
- HTML  
- CSS  
- JavaScript  
- SQL  

#### Tools
- Postman  
- cURL  

---

## 🧠 Summary
This system combines:
- 📷 Computer vision (ANPR / OCR / LPR)  
- 📡 Sensors (proximity detection)  
- 💳 RFID payments  
- 🚨 Security monitoring  
- 🌐 Web dashboard  