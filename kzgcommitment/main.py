import math
from hashlib import sha256


def id_to_bin(id2: int, levels) -> str:
    return bin(id2)[2:].zfill(levels)

def hash_val(val: str) -> str:
    return sha256(val.encode()).hexdigest()

def hash_verify(hash:str, input:str) -> bool:
    return sha256(input.encode()).hexdigest() == hash

default_hash = hash_val("0")

class sparse_merkle_tree:
    def __init__(self):
        self.levels = 5 # 32 node storage
        self.nodes = {}
    def add_node(self, id, domain):
        self.nodes[id] = [domain, {}]
        binary = id_to_bin(id, self.levels - 1)
        level = self.levels
        relevant_proofs = {}
        prevHash = None
        prevSibling = None
        for i in reversed(binary):
            node = int(i)
            sibling = node ^ 1
            siblingPath = f"{binary[:level-2]}{sibling}"
            found = False
            for i in self.nodes:
                if(siblingPath in self.nodes[i][1]):
                    found = True
                    relevant_proofs[siblingPath] = self.nodes[i][1][siblingPath]
                    break
            if not found:
                relevant_proofs[siblingPath] = hash_val("0")
            if level == self.levels:
                prevHash = hash_val(domain)
            else:
                prevHash = hash_val(prevHash + relevant_proofs[prevSibling])
            for k in self.nodes:
                if(binary[:level-1] in self.nodes[k][1]):
                    self.nodes[k][1][binary[:level-1]] = prevHash
            level -= 1
            prevSibling = siblingPath
        #if(prevSibling):
        #    prevHash = hash_val(prevHash + relevant_proofs[prevSibling])
        #print(prevHash)
        relevant_proofs[binary] = hash_val(domain)
        self.nodes[id][1] = relevant_proofs
        for k in self.nodes:
            self.nodes[k][1]["root"] = prevHash
        return relevant_proofs, prevHash
    def __str__(self):
        return str(self.nodes)
    def get_relevant_proofs(self, id):
        return self.nodes[id][1]
    def get_actual_root(self):
        print(hash_val(hash_val(hash_val(hash_val(hash_val("Bob") + hash_val("Alicia")) + hash_val("0")) + hash_val("0"))+ hash_val("0")))
    def verify_domain(self, id, domain, relevant_proofs):
        binary = id_to_bin(id, self.levels - 1)
        prev_hash = None
        for i in reversed(range(self.levels)):
             if i == self.levels - 1:
                prev_hash = hash_val(domain)
             else:
                siblingPath = f"{binary[:i]}{int(binary[i]) ^ 1}"
                prev_hash = hash_val(prev_hash + relevant_proofs[siblingPath])
                #print(siblingPath, i)
        return prev_hash




dict = {
    0: "Bob",
    1: "Alicia",
    2: "Ally",
    3: "Christopher",
    6: "Nick"
}

tree = sparse_merkle_tree()
tree.add_node(0, dict[0])
tree.add_node(1, dict[1])
#tree.add_node(4, dict[6])
print(tree)
relevant = tree.get_relevant_proofs(0)
relevant2 = tree.get_relevant_proofs(1)
print(relevant2["root"])
print(tree.verify_domain(0, "Bob", relevant))
print(tree.get_actual_root())



