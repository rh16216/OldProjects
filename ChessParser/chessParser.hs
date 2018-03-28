import Yoda
import Data.Monoid
import Data.Char

data Chess = Turn Move Move Chess
           | EndGame
  deriving Show

data Move = Move MoveP Quant
          | Cslt Bool
          | End Winner
  deriving Show

data Quant = Prom Piece Quant
           | Chck Quant
           | Null
  deriving Show

data MoveP = Alg Piece Cell
           | Smh Cell Cell
           | AlgDis Piece Cell Cell
           | Tke Piece Cell
  deriving Show

data Winner = White
            | Black
            | Draw
            | AO
  deriving Show

data Cell = Cell Char Int
  deriving (Show, Eq)

data Piece = King
           | Queen
           | Rook
           | Knight
           | Bishop
           | Pawn
  deriving (Show, Eq)

pieceParser :: Parser Piece
pieceParser =  King   <$ string "K"
           <|> Queen  <$ string "Q"
           <|> Rook   <$ string "R"
           <|> Knight <$ string "N"
           <|> Bishop <$ string "B"
           <|> pure Pawn

cellParser :: Parser Cell
cellParser = Cell <$> oneOf['a'..'h'] <*> (digitToInt <$> oneOf['1'..'8'])

winnerParser :: Parser Winner
winnerParser =  White <$ string "1-0"
            <|> Black <$ string "0-1"
            <|> Draw  <$ string "1/2 - 1/2"
            <|> pure AO

whitespace :: Parser ()
whitespace = () <$ many (oneOf " \t")

movePParser :: Parser MoveP
movePParser =  Alg <$> pieceParser <*> cellParser
           <|> Smh <$> cellParser <*> cellParser
           <|> AlgDis <$> pieceParser <*> cellParser <*> cellParser
           <|> Tke <$> pieceParser  <* string "x" <*> cellParser

promPieceParser :: Parser Piece
promPieceParser =  Queen  <$ string "Q"
               <|> Rook   <$ string "R"
               <|> Knight <$ string "N"
               <|> Bishop <$ string "B"

quantParser :: Parser Quant
quantParser =  Prom <$> promPieceParser <*> quantParser
           <|> Chck <$ (string "+" <|> string "#") <*> quantParser
           <|> pure Null

moveParser :: Parser Move
moveParser =  Move <$> movePParser <*> quantParser <* whitespace
          <|> Cslt False  <$ string "0-0" <* whitespace
          <|> Cslt True <$ string "0-0-0" <* whitespace

endMoveParser :: Parser Move
endMoveParser =  Move <$> movePParser <*> quantParser <* whitespace
          <|> Cslt False  <$ string "0-0" <* whitespace
          <|> Cslt True <$ string "0-0-0" <* whitespace
          <|> End <$> winnerParser <* whitespace


chessParser :: Parser Chess
chessParser =  Turn <$ number <* tok "." <*> moveParser <*> endMoveParser <*> chessParser
           <|> EndGame <$ eof
