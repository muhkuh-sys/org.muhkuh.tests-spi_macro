# **SPI Macro Synthax Description**

## **Commands**

---

### **Comments**

Comments are possible as single line:
```
# Comment
```
or after a command:
```
CMD(<parameters>) # Comment
```

---

### **CS Command**

**Synthax:**
```
CS(<Enable/disable chip select mode>) 
```
**Parameters:**
- `<Enable/disable chip select mode>` :
  - true (active CS)
  - false (deactive CS)

---

### **Receive Command**

**Synthax:**
```
RECEIVE(<number of received bytes>)
```

---

### **Send Command**

**Synthax:**
```
SEND(<data to be transferred>,...)
```

---

### **Idle Command**

**Synthax:**
```
IDLE(<number of idle cycles>)
```

---

### **Dummy Command**

**Synthax:**
```
DUMMY(<number of dummy bytes>)
```

---

### **Jump Command**

**Synthax:**
```
JUMP(<condition name>,<name of label>)
```

**Parameters:**
- `<name of label>` :
  - string with "" or ''
- `<condition name>` :
  - NotEqual
  - Always
  - Zero
  - NotZero
  - Equal

---

### **CHTR Command**

---

### **Compare Command**

**Synthax:**
```
CMP(<data to be compared>,...)
```

---

### **Mask Command**

**Synthax:**
```
MASK(<data to be masked>,...)
```

---

### **Mode Command**

**Synthax:**
```
MODE(<selected bus width mode>)
```
**Parameters:**
- `<selected mode>` :
  - 1BIT
  - 2BIT
  - 4BIT

---

### **ADR Command**

---

### **Fail Command**

**Synthax:**
```
FAIL(<condition name>,<Error Message>)
```
**Parameters:**
- `<Error Message>` :
  - string with "" or ''
- `<condition name>` :
  - NotEqual
  - Always
  - Zero
  - NotZero
  - Equal


---

### **Examples:**

**equivalent of SMC_SEND, SMCS_SNN, 1, 0x9f:**

CS(true)
SEND(0x9f)

**equivalent of SMC_RECEIVE, SMCS_SDD, 5:**

CS(true)
RECEIVE(5)
CS(false)
DUMMY(1)

**equivalent of SMC_SEND, SMCS_SDN, 1, 0x9f :**

CS(true)
SEND(0x9f)
CS(false)

**equivalent of SMC_DUMMY, SMCS_NNN, 2:**

DUMMY(2)

---