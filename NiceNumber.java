package XL;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.apache.pig.EvalFunc;
import org.apache.pig.data.Tuple;
import org.apache.pig.data.TupleFactory;


public class NiceNumber extends EvalFunc<Tuple> {

	@Override
	public Tuple exec(Tuple input) throws IOException {
		if(input == null || input.size() == 0)
			return null;
		try {
			String value = (String) input.get(0);
			HashMap<Character,Integer> m = repeatedSequences(value);
			Iterator it = m.entrySet().iterator();
			int maxCount = -1;
			char maxChar = ' ';
			int nPairs= 0;
			while(it.hasNext())  {
				Map.Entry pair = (Map.Entry) it.next();
				if((Integer)pair.getValue() > maxCount) {
					maxCount = (Integer)pair.getValue();
					maxChar =(Character)pair.getKey();
					if(maxCount >= 2)
						nPairs ++;
				}
					
			}
			
			Tuple t  = TupleFactory.getInstance().newTuple(3);
			
			t.set(0,""+maxChar);
			t.set(1,(float)maxCount);
			t.set(2,value.contains("4"));

			return t;
			
		} catch(Exception e) {
			System.err.println("Failed to process the input, is it a string?\n"+e.getMessage());
			throw e;
		}
	}
	public static HashMap<Character,Integer> repeatedSequences(String s) {
		HashMap<Character,Integer> counts  = new HashMap<Character,Integer>();
		
		char[] charArr = s.toCharArray();
		char lastChar = '$';
		int count = 1;
		for(int i = 0; i < charArr.length;i++) {
			if(charArr[i] != lastChar) {
				lastChar = new Character(charArr[i]);
				count = 1;
			}
			else {
				count++;
				counts.put(lastChar,count);
			}
		}
		return counts;
	}
	public static void main(String[] args) {
	}
}
