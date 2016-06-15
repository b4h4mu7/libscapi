#pragma once
#include "CommitmentScheme.hpp"
#include "../mid_layer/ElGamalEnc.hpp"

/**
* This class holds the values used by the ElGamal Committer during the commitment phase
* for a specific value that the committer commits about.
* This value is kept attached to a random value used to calculate the commitment,
* which is also kept together in this structure.
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalCommitmentPhaseValues : public CmtCommitmentPhaseValues {
private:
	//The value that the committer sends to the receiver in order to commit commitval in the commitment phase.
	shared_ptr<AsymmetricCiphertext> computedCommitment;

public:
	/**
	* Constructor that sets the given random value, committed value and the commitment object.
	* This constructor is package private. It should only be used by the classes in the package.
	* @param r random value used for commit.
	* @param commitVal the committed value
	* @param computedCommitment the commitment
	*/
	CmtElGamalCommitmentPhaseValues(shared_ptr<RandomValue> r, shared_ptr<CmtCommitValue> commitVal, shared_ptr<AsymmetricCiphertext> computedCommitment) 
		: CmtCommitmentPhaseValues(r, commitVal) {
		this->computedCommitment = computedCommitment;
	}

	/**
	* Returns the value that the committer sends to the receiver in order to commit
	* commitval in the commitment phase.
	* @return the commitment value
	*/
	shared_ptr<void> getComputedCommitment() override {	return computedCommitment; }
};

/**
* Concrete implementation of commitment message used by ElGamal commitment scheme.
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalCommitmentMessage : public CmtCCommitmentMsg {

	// In ElGamal schemes the commitment object is a ElGamalCiphertext
	//In order to this class be a serializable, we get it as ElGamalCiphertextSendableData. 
private:
	shared_ptr<ElGamalOnGrElSendableData> cipherData;
	long id; //The id of the commitment

public:
	/**
	* Constructor that sets the commitment and id.
	* @param cipherData the actual commitment object. In ElGamal schemes the commitment object is a ElGamalCiphertextSendableData.
	* @param id the commitment id.
	*/
	CmtElGamalCommitmentMessage(shared_ptr<ElGamalOnGrElSendableData> cipherData = NULL, long id = 0) {
		this->cipherData = cipherData;
		this->id = id;
	}

	/**
	* Returns the commitment value. In this case the instance of the commitment is ElGamalCiphertextSendableData.
	*/
	shared_ptr<void> getCommitment() {	return cipherData; }

	/**
	* Returns the commitment id.
	*/
	long getId() { return id; }

	// network serialization implementation:
	void initFromString(const string & s) override {
		auto vec = explode(s, ':');
		id = stol(vec[0]);
		string inner = "";
		for (int i = 0; i < vec.size() - 1; i++) {
			inner += vec[1 + i];
			inner += ":";
		}
		cipherData->initFromString(inner);
	}
	string toString() override { return to_string(id) + ":" + cipherData->toString(); };
};

/**
* Concrete implementation of decommitment message used by ElGamal commitment scheme.
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalDecommitmentMessage : public CmtCDecommitmentMessage {


private:
	//This message is common to ElGamal on GroupElement and on byte[].
	//In order to enable this, x value can hold every serializable object.
	string x;
	shared_ptr<BigIntegerRandomValue> r; //Random value sampled during the sampleRandomValues stage;

public:
	/**
	* Constructor that sets the given committed value and random value.
	* @param x the committed value
	* @param r the random value used for commit.
	*/
	CmtElGamalDecommitmentMessage(string x = "", shared_ptr<BigIntegerRandomValue> r = NULL) {
		this->x = x;
		this->r = r;
	}

	/**
	* Returns the committed value.
	*/
	string getX() { return x;	}

	/**
	* Returns the random value used for commit.
	*/
	shared_ptr<RandomValue> getR() override { return r;	}

	// network serialization implementation:
	void initFromString(const string & s) override {
		auto vec = explode(s, ':');
		if (vec.size() == 2) {
			x = vec[0];
			r = make_shared<BigIntegerRandomValue>(biginteger(vec[1]));
		} else if (vec.size() == 3) {
			x = vec[0] + ":" + vec[1];
			r = make_shared<BigIntegerRandomValue>(biginteger(vec[2]));
		} 
	}

	string toString() override { return x + ":" + (string) r->getR(); };
};

/**
* This abstract class performs all the core functionality of the committer side of
* ElGamal commitment.
* Specific implementations can extend this class and add or override functions as necessary.
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalCommitterCore : public CmtCommitter {
	/*
	* runs the following protocol:
	* "Commit phase
	*		IF NOT VALID_PARAMS(G,q,g)
	*			REPORT ERROR and HALT
	*		SAMPLE random values  a,r <- Zq
	*		COMPUTE h = g^a
	*		COMPUTE u = g^r and v = h^r * x
	*		SEND c = (h,u,v) to R
	*	Decommit phase
	*		SEND (r, x)  to R
	*		OUTPUT nothing"
	*
	*/

private:
	/**
	* Sets the given parameters and execute the preprocess phase of the scheme.
	* @param channel
	* @param dlog
	* @param elGamal
	* @param random
	* @throws SecurityLevelException if the given dlog is not DDH secure
	* @throws InvalidDlogGroupException if the given dlog is not valid.
	* @throws IOException if there was a problem in the communication
	*/
	void doConstruct(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<ElGamalOnGroupElementEnc> elGamal);

	/**
	* The pre-process is performed once within the construction of this object.
	* If the user needs to generate new pre-process values then it needs to disregard
	* this instance and create a new one.
	* Runs the following lines from the pseudo code:
	* "SAMPLE random values  a<- Zq
	*	COMPUTE h = g^a"
	* @throws IOException
	*/
	void preProcess();

protected:
	shared_ptr<DlogGroup> dlog;
	mt19937 random;
	biginteger qMinusOne;
	shared_ptr<ElGamalOnGroupElementEnc> elGamal;
	shared_ptr<ElGamalPublicKey> publicKey;
	shared_ptr<ElGamalPrivateKey> privateKey;

	/**
	* Constructor that receives a connected channel (to the receiver),
	* the DlogGroup agreed upon between them, the encryption object and a SecureRandom.
	* The Receiver needs to be instantiated with the same DlogGroup,
	* otherwise nothing will work properly.
	*/
	CmtElGamalCommitterCore(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<ElGamalOnGroupElementEnc> elGamal) {
		doConstruct(channel, dlog, elGamal);
	}

public:

	/**
	* Computes the commitment object of the commitment scheme. <p>
	* Pseudo code:<p>
	* "SAMPLE random values  r <- Zq <p>
	*	COMPUTE u = g^r and v = h^r * x". <p>
	* @return the created commitment.
	*/
	shared_ptr<CmtCCommitmentMsg> generateCommitmentMsg(shared_ptr<CmtCommitValue> input, long id) override;

	shared_ptr<CmtCDecommitmentMessage> generateDecommitmentMsg(long id) override; 

	vector<shared_ptr<void>> getPreProcessValues() override;
};



/**
* This class implements the committer side of the ElGamal commitment. <p>
* It uses El Gamal encryption for  group elements, that is, the encryption class used is
* ScElGamalOnGroupElement. This default cannot be changed.<p>
*
* The pseudo code of this protocol can be found in Protocol 3.4 of pseudo codes document at {@link http://cryptobiu.github.io/scapi/SDK_Pseudocode.pdf}.<p>
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalOnGroupElementCommitter : public CmtElGamalCommitterCore, public PerfectlyBindingCmt, public CmtOnGroupElement {

public:
	/**
	* This constructor lets the caller pass the channel and the dlog group to work with. The El Gamal option (ScElGamalOnGroupElement)is set by default by the constructor and cannot be changed.
	* @param channel used for the communication
	* @param dlog	Dlog group
	* @throws IllegalArgumentException
	* @throws SecurityLevelException
	* @throws InvalidDlogGroupException
	* @throws IOException
	*/
	CmtElGamalOnGroupElementCommitter(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog = make_shared<OpenSSLDlogECF2m>("K-233"))
		: CmtElGamalCommitterCore(channel, dlog, make_shared<ElGamalOnGroupElementEnc>(dlog)) {}

	shared_ptr<CmtCCommitmentMsg> generateCommitmentMsg(shared_ptr<CmtCommitValue> input, long id) override; 

	/**
	* This function samples random commit value and returns it.
	* @return the sampled commit value
	*/
	shared_ptr<CmtCommitValue> sampleRandomCommitValue() override {
		return make_shared<CmtGroupElementCommitValue>(dlog->createRandomElement());
	}

	shared_ptr<CmtCommitValue> generateCommitValue(vector<byte> x) override {
		throw UnsupportedOperationException("El Gamal committer cannot generate a CommitValue from a byte[], since there isn't always a suitable encoding");
	}

	/**
	* This function converts the given commit value to a byte array.
	* @param value
	* @return the generated bytes.
	*/
	vector<byte> generateBytesFromCommitValue(CmtCommitValue* value) override;

};

/**
* This abstract class performs all the core functionality of the receiver side of
* ElGamal commitment.
* Specific implementations can extend this class and add or override functions as necessary.
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalReceiverCore : public CmtReceiver {

	/*
	* runs the following protocol:
	* "Commit phase
	*		WAIT for a value c
	*		STORE c
	*	Decommit phase
	*		WAIT for (r, x)  from C
	*		Let c = (h,u,v); if not of this format, output REJ
	*		IF NOT
	*			VALID_PARAMS(G,q,g), AND
	*			h <-G, AND
	*			u=g^r
	*			v = h^r * x
	*			x in G
	*		      OUTPUT REJ
	*		ELSE
	*		      OUTPUT ACC and value x"
	*
	*/
private:
	/**
	* Sets the given parameters and execute the preprocess phase of the scheme.
	* @param channel
	* @param dlog
	* @param elGamal
	*/
	void doConstruct(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<ElGamalOnGroupElementEnc> elGamal);

	/**
	* The pre-process is performed once within the construction of this object.
	* If the user needs to generate new pre-process values then it needs to disregard
	* this instance and create a new one.
	*/
	void preProcess();

protected:
	shared_ptr<DlogGroup> dlog;
	shared_ptr<CommParty> channel;
	shared_ptr<ElGamalOnGroupElementEnc> elGamal;
	shared_ptr<ElGamalPublicKey> publicKey;

public:
	/**
	* Constructor that receives a connected channel (to the receiver),
	* the DlogGroup agreed upon between them and the encryption object.
	* The committer needs to be instantiated with the same DlogGroup,
	* otherwise nothing will work properly.
	*/
	CmtElGamalReceiverCore(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<ElGamalOnGroupElementEnc> elGamal) {
		doConstruct(channel, dlog, elGamal);
	}

	/**
	* Runs the commit phase of the commitment scheme.<p>
	* Pseudo code:<p>
	* "WAIT for a value c<p>
	*	STORE c".
	* @return the output of the commit phase.
	*/
	shared_ptr<CmtRCommitPhaseOutput> receiveCommitment() override; 

	/**
	* Runs the decommit phase of the commitment scheme.<p>
	* Pseudo code:<p>
	* "WAIT for (r, x)  from C<p>
	*	Let c = (h,u,v); if not of this format, output REJ<p>
	*	IF NOT<p>
	*		u=g^r <p>
	*		v = h^r * x<p>
	*		x in G<p>
	*		OUTPUT REJ<p>
	*	ELSE<p>
	*	    OUTPUT ACC and value x"
	* @param id
	* @return the committed value if the decommit succeeded; null, otherwise.
	*/
	shared_ptr<CmtCommitValue> receiveDecommitment(long id) override; 

	vector<shared_ptr<void>> getPreProcessedValues() override;
};

/**
* This class implements the receiver side of the ElGamal commitment. <p>
* It uses El Gamal encryption for  group elements, that is, the encryption class used is
* ScElGamalOnGroupElement. This default cannot be changed.<p>
*
* The pseudo code of this protocol can be found in Protocol 3.4 of pseudo codes document at {@link http://cryptobiu.github.io/scapi/SDK_Pseudocode.pdf}.<p>
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtElGamalOnGroupElementReceiver : public CmtElGamalReceiverCore, public PerfectlyBindingCmt, public CmtOnGroupElement {

public:
	/**
	* This constructor lets the caller pass the channel and the dlog group to work with.
	* The El Gamal option (ScElGamalOnGroupElement)is set by default by the constructor
	* and cannot be changed.
	* @param channel used for the communication
	* @param dlog Dlog group
	*/
	CmtElGamalOnGroupElementReceiver(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog = make_shared<OpenSSLDlogECF2m>("K-233"))
		: CmtElGamalReceiverCore(channel, dlog, make_shared<ElGamalOnGroupElementEnc>(dlog)) {}

	/**
	* Proccesses the decommitment phase.<p>
	* "IF NOT<p>
	*		u=g^r <p>
	*		v = h^r * x<p>
	*		x in G<p>
	*		OUTPUT REJ<p>
	*	ELSE<p>
	*	    OUTPUT ACC and value x"<p>
	* @param id the id of the commitment.
	* @param msg the receiver message from the committer
	* @return the committed value if the decommit succeeded; null, otherwise.
	*/
	shared_ptr<CmtCommitValue> verifyDecommitment(CmtCCommitmentMsg* commitmentMsg,
		CmtCDecommitmentMessage* decommitmentMsg) override; 

	/**
	* This function converts the given commit value to a byte array.
	* @param value
	* @return the generated bytes.
	*/
	vector<byte> generateBytesFromCommitValue(CmtCommitValue* value) override;

};
